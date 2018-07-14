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

Move Node::next_move()
{
    Move m = mlist.back();
    mlist.pop_back();
    return m;
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
    child.set_move(move);
    child.generate_moves();
    children.push_back(child);
    return &children.back();
}

template <>
double Node::score<VALUE>(u64 parent_simulations)
{
    return double(wins) / double(simulations);
}

template <>
double Node::score<SELECTION>(u64 parent_simulations)
{
    return double(wins) / double(simulations)
        + std::sqrt(2 * std::log(double(parent_simulations)) / double(simulations));
}

template <Policy policy>
Node* Node::get_child()
{
    std::size_t best_index = 0;
    double best_score = children[best_index].score<policy>(simulations);
    for (std::size_t i = 1; i < children.size(); ++i) {
        double score = children[i].score<policy>(simulations);
        if (score > best_score)
        {
            best_score = score;
            best_index = i;
        }
    }
    return &children[best_index];
}

int Node::simulate()
{
    Position pos = this->pos;

    std::vector<Move> local_mlist;
    local_mlist.reserve(256);
    while (true) {
        if (pos.is_drawn())
            return DRAW;

        // Generate moves for position
        local_mlist.clear();
        pos.generate_legal_movelist(local_mlist);
        if (local_mlist.empty())
            break;

        // Play a random move
        int r = utils::rand_int(0, local_mlist.size() - 1);
        Move& move = local_mlist[r];
        pos.make_move(move);
    }

    int result;
    if (!pos.checkers_to(US))
    {
        result = DRAW;
    }
    else
    {
        if (this->get_position().is_flipped() != pos.is_flipped())
            result = LOSS;
        else
            result = WIN;
    }

    return result;
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

void backprop(const std::vector<Node*>& parents, int result)
{
    bool flipper = true;
    for (int i = parents.size()-1; i >= 0; --i) {
        Node* parent = parents[i];
        parent->inc_simulations();
        if (   (flipper && result == LOSS)
            || (!flipper && result == WIN))
        {
            parent->inc_wins();
        }
        flipper = !flipper;
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

        int result = curr->simulate();
        if (result == WIN)
            curr->inc_wins();
        curr->inc_simulations();
        backprop(parents, result);

        time_ms now = utils::curr_time();
        if (now - prev_print_time >= 500)
        {
            std::vector<Move> pv = this->pv();
            Node* best_child = root.get_child<VALUE>();
            std::cout << "info"
                << " cp " << 100.0 * best_child->score<VALUE>() << "%"
                << " nodes " << root.get_simulations()
                << " time " << now - start_time
                << " nps " << root.get_simulations() * 1000 / (now - start_time)
                << " pv " << uci::get_pv_string(pv, root_flipped)
                << std::endl;
            prev_print_time = now;
        }
    }

    Move best_move = root.get_child<VALUE>()->get_move();
    std::cout << "bestmove " << get_move_string(best_move, root_flipped) << std::endl;
}
