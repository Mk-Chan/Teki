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

#include "lc0nn.h"

Move Node::next_move()
{
    Move m = mlist.back();
    mlist.pop_back();
    return m;
}

float Node::next_p()
{
    float p = children_p.back();
    children_p.pop_back();
    return p;
}

bool Node::is_terminal()
{
    return mlist.empty() && children.empty();
}

bool Node::expanded()
{
    return !children.empty();
}

bool Node::has_moves()
{
    return !mlist.empty();
}

Node* Node::expand()
{
    Position p = pos;
    Move move = next_move();
    p.make_move(move);
    Node child {p};
    child.set_p(next_p());
    child.set_move(move);
    child.generate_moves();
    children.push_back(child);
    return &children.back();
}

template <>
double Node::score<VALUE>(int parent_visits)
{
    return double(visits);
}

template <>
double Node::score<SELECTION>(int parent_visits)
{
    float x = visits ? double(wins) / double(visits) : 0;
    float c = visits
        ? std::sqrt((3.0 / 2.0) * std::log(double(parent_visits)) / double(visits))
        : 0;
    float m = 2.0 / self_p;
    if (parent_visits > 1)
        m *= std::sqrt(std::log(double(parent_visits)) / double(parent_visits));

    return x + c - m;
}

template <Policy policy>
Node* Node::get_child()
{
    std::size_t best_index = 0;
    double best_score = children[best_index].score<policy>(visits);
    for (std::size_t i = 1; i < children.size(); ++i) {
        double score = children[i].score<policy>(visits);
        if (score > best_score)
        {
            best_score = score;
            best_index = i;
        }
    }
    return &children[best_index];
}

void Node::compute(std::vector<Node*>& parents)
{
    Position pos = this->pos;

    if (pos.is_drawn())
    {
        set_q(0);
        return;
    }

    generate_moves();
    const std::vector<Move>& mlist = get_mlist();
    if (mlist.empty())
    {
        set_q(pos.checkers_to(US) ? 1 : 0);
        return;
    }

    auto pos_hist = PositionHistory {};
    pos_hist.Reset(pos);
    auto comp = network->NewComputation();
    auto planes = lczero::EncodePositionForNN(pos_hist);
    comp->AddInput(std::move(planes));
    comp->ComputeBlocking();

    set_q(comp->GetQVal(0));
    for (const Move& m : mlist) {
        std::string move_str = get_move_string(m, false);
        push_p(comp->GetPVal(0, lc0_move_index(move_str)));
    }
}

std::pair<Node*, std::vector<Node*>> GameTree::select()
{
    std::vector<Node*> parents;
    Node* curr = &root;
    while (!curr->is_terminal()) {
        if (curr->has_moves())
        {
            parents.push_back(curr);
            curr = curr->expand();
            break;
        }
        parents.push_back(curr);
        curr = curr->get_child<SELECTION>();
    }

    return {curr, parents};
}

void backprop(const std::vector<Node*>& parents, float child_q)
{
    float prev_q = 0.0;
    for (int i = parents.size()-1; i >= 0; --i) {
        Node* parent = parents[i];
        float q = parent->get_q();
        if (parent->flipped())
            q = -q;
        int visits = parent->get_visits();
        parent->set_q((q * visits - prev_q + child_q) / (visits + 1));
        parent->inc_visits();
        prev_q = q;
    }
}

std::vector<Move> GameTree::pv()
{
    std::vector<Move> pv;
    Node* curr = &root;
    while (curr->expanded()) {
        curr = curr->get_child<VALUE>();
        pv.push_back(curr->get_move());
    }
    return pv;
}

void GameTree::search()
{
    time_ms start_time = utils::curr_time();
    time_ms prev_print_time = start_time;
    bool root_flipped = root.get_position().is_flipped();

    while (!stopped()) {
        auto selection = select();
        auto& [curr, parents] = selection;

        curr->compute(parents);
        curr->inc_visits();
        backprop(parents, curr->get_q());

        time_ms now = utils::curr_time();
        if (now - prev_print_time >= 500)
        {
            std::vector<Move> pv = this->pv();
            Node* best_child = root.get_child<VALUE>();
            int cp = 290.680623072 * std::tan(1.548090806 * best_child->get_q());
            std::cout << "info"
                << " cp " << cp
                << " nodes " << root.get_visits()
                << " time " << now - start_time
                << " nps " << root.get_visits() * 1000 / (now - start_time)
                << " pv " << uci::get_pv_string(pv, root_flipped)
                << std::endl;
            prev_print_time = now;
        }
    }

    Move best_move = root.get_child<VALUE>()->get_move();
    std::cout << "bestmove " << get_move_string(best_move, root_flipped) << std::endl;
}
