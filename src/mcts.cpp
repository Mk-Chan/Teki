/*
MIT License

Copyright (c) 2018 Manik Charan

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
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
    return double(wins) / double(simulations) +
            std::sqrt(2 * std::log(double(parent_simulations)) / double(simulations));
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
    bool root_flipped = root.get_position().is_flipped();
    u64 prev_simulations = 0;

    while (!stopped()) {
        auto selection = select();
        auto& [curr, parents] = selection;

        int result = curr->simulate();
        if (result == WIN)
            curr->inc_wins();
        curr->inc_simulations();
        backprop(parents, result);

        if (root.get_simulations() - prev_simulations >= 10000)
        {
            std::vector<Move> pv = this->pv();
            Node* best_child = root.get_child<VALUE>();
            time_ms now = utils::curr_time();
            std::cout << "info"
                << " cp " << 100.0 * best_child->get_wins() / double(best_child->get_simulations()) << "%"
                << " nodes " << root.get_simulations()
                << " time " << now - start_time
                << " nps " << root.get_simulations() * 1000 / (now - start_time)
                << " pv " << uci::get_pv_string(pv, root_flipped)
                << std::endl;
            prev_simulations = root.get_simulations();
        }
    }

    Move best_move = root.get_child<VALUE>()->get_move();
    std::cout << "bestmove " << get_move_string(best_move, root_flipped) << std::endl;
}
