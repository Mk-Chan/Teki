/*
 * Teki-MCTS
 * Copyright (C) 2018  Manik Charan
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>

#include "uci.h"
#include "mcts.h"
#include "move.h"
#include "options.h"
#include "controller.h"

float c_puct = 1.22;

Node* Node::most_visited_child()
{
    return &*std::max_element(_children.begin(), _children.end(), [](auto a, auto b) {
        return a.visits() < b.visits();
    });
}

float Node::ucb(float parent_visits) const
{
    return _payoff + c_puct * _prior * std::sqrt(parent_visits) / (1.0f + _visits);
}

void Node::update_payoff(float child_payoff)
{
    _payoff += (child_payoff - _payoff) / _visits;
}

void Node::compute(std::vector<Node*>& parents, const PositionHistory& pos_hist)
{
    std::vector<Move> mlist;
    mlist.reserve(256);
    _pos.generate_legal_movelist(mlist);
    if (mlist.empty())
    {
        _payoff = _pos.checkers_to(US) ? -1.0f : 0.0f;
        return;
    }

    PositionHistory local_pos_hist;
    bool is_flipped = flipped();
    for (int i = 0; i < pos_hist.GetLength()-1; ++i) {
        Position p {pos_hist.GetPositionAt(i)};
        if (is_flipped ^ p.is_flipped())
            p.flip();
        local_pos_hist.Append(p);
    }
    if (!parents.empty())
    {
        for (int i = 0; i < parents.size(); ++i) {
            Position p {parents[i]->pos()};
            if (is_flipped ^ p.is_flipped())
                p.flip();
            local_pos_hist.Append(p);
        }
    }
   local_pos_hist.Append(_pos);
    local_pos_hist.TrimTo8();
    auto planes = lczero::EncodePositionForNN(local_pos_hist);
    auto comp = network->NewComputation();
    comp->AddInput(std::move(planes));
    comp->ComputeBlocking();
    ++controller.nodes_searched;

    _payoff = comp->GetQVal(0);
    std::vector<float> priors;
    for (Move& move : mlist) {
        std::string move_str {get_move_string(move, false)};
        if (move_str == "e1g1")
            move_str = "e1h1";
        else if (move_str == "e1c1")
            move_str = "e1a1";
        priors.push_back(comp->GetPVal(0, lc0_move_index(move_str)));
    }
    priors = softmax(priors);

    //std::cout << "Q: " << comp->GetQVal(0) << std::endl;
    for (int i = 0; i < mlist.size(); ++i) {
        Move& move = mlist[i];
        float prior = priors[i];
        Position child_pos {_pos};
        child_pos.make_move(move);
        std::string mstr {get_move_string(move, _pos.is_flipped())};
        //std::cout << mstr << " P: " << prior;
        Node child {child_pos, prior, _payoff, move};
        //std::cout << " U: " << child.ucb(_visits) << std::endl;
        _children.push_back(child);
    }
    if (_pos.is_drawn())
        _payoff = 0.0f;
}

std::pair<Node*, std::vector<Node*>> GameTree::select()
{
    Node* curr = &root;
    std::vector<Node*> parents;
    while (curr->expanded()) {
        parents.push_back(curr);
        std::vector<Node>& children = curr->children();
        float best_ucb = -30000.0f;
        Node* best_child;
        for (auto& child : children) {
            float child_ucb = child.ucb(curr->visits());
            if (child_ucb > best_ucb)
            {
                best_ucb = child_ucb;
                best_child = &child;
            }
        }
        curr = best_child;
    }

    return {curr, parents};
}

void backprop(std::vector<Node*>& parents, float child_payoff)
{
    for (auto par = parents.rbegin(); par != parents.rend(); ++par) {
        (*par)->inc_visits();
        (*par)->update_payoff(child_payoff);
        child_payoff = -child_payoff;
    }
}

std::vector<Move> GameTree::pv()
{
    std::vector<Move> pv;
    Node* curr = &root;
    while (curr->expanded()) {
        curr = curr->most_visited_child();
        pv.push_back(curr->move());
    }
    return pv;
}

void GameTree::search()
{
    time_ms start_time = utils::curr_time();
    time_ms prev_print_time = start_time;
    bool root_flipped = root.flipped();
    controller.nodes_searched = 0;

    while (!stopped()) {
        auto& [curr, parents] = select();
        curr->compute(parents, pos_hist);
        curr->inc_visits();
        backprop(parents, curr->payoff());

        time_ms now = utils::curr_time();
        if (now - prev_print_time >= 1000)
        {
            std::vector<Move> pv = this->pv();
            Node* best_child = root.most_visited_child();
            int cp = 290.680623072 * std::tan(1.548090806 * best_child->payoff());
            std::cout << "info"
                << " score cp " << cp
                << " nodes " << root.visits()
                << " time " << now - start_time
                << " nps " << root.visits() * 1000 / (now - start_time)
                << " pv " << uci::get_pv_string(pv, root_flipped)
                << std::endl;
            prev_print_time = now;
        }
    }

    Move best_move = root.most_visited_child()->move();
    std::cout << "bestmove " << get_move_string(best_move, root_flipped) << std::endl;
}
