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

bool Node::fully_expanded()
{
    return expanded && mlist.empty();
}

bool Node::leaf_node()
{
    return !expanded;
}

void Node::expand()
{
    if (leaf_node()) {
        assert(mlist.empty());
        generate_children();
        expanded = true;
    }
    Position p = pos;
    Move move = next_move();
    p.make_move(move);
    children.emplace_back(p);
    children.back().set_move(move);
}

double Node::value_score()
{
    return double(wins) / double(simulations);
}

double Node::selection_score(u64 parent_simulations)
{
    return double(wins) / double(simulations) +
            std::sqrt(2 * std::log(double(parent_simulations)) / double(simulations));
}

Node* Node::best_child()
{
    int best_index = 0;
    double best_score = children[best_index].value_score();
    for (std::size_t i = 1; i < children.size(); ++i) {
        double score = children[i].value_score();
        if (score > best_score)
        {
            best_score = score;
            best_index = i;
        }
    }
    return &children[best_index];
}

Node* Node::select_child()
{
    int best_index = 0;
    double best_score = children[best_index].selection_score(simulations);
    for (std::size_t i = 1; i < children.size(); ++i) {
        double score = children[i].selection_score(simulations);
        if (score > best_score)
        {
            best_score = score;
            best_index = i;
        }
    }
    return &children[best_index];
}

int Node::simulate(int original_stm)
{
    ++simulations;
    Position pos = this->pos;

    int terminal_stm = original_stm;

    std::vector<Move> local_mlist;
    local_mlist.reserve(256);
    bool first = true;
    int depth = 0;
    int result;
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
        terminal_stm = !terminal_stm;
        first = false;
        ++depth;
    }

    // Return 0 for a draw
    if (!pos.checkers_to(US))
    {
        result = DRAW;
    }
    else
    {
        int curr_stm = this->pos.is_flipped() ? 1 : 0;
        if ((terminal_stm == original_stm) ^ (curr_stm == original_stm))
        {
            result = WIN;
            ++wins;
        }
        else
        {
            result = LOSS;
        }
    }

    if (first)
        this->result = TERMINAL;

    return result;
}

std::pair<Node*, std::vector<Node*>> GameTree::select()
{
    std::vector<Node*> parents;
    Node* curr = &root;
    while (curr->fully_expanded()) {
        parents.push_back(curr);
        curr = curr->select_child();
    }
    return {curr, parents};
}

void backprop(const std::vector<Node*>& parents, int result)
{
    int flipper = 1;
    for (int i = parents.size()-1; i >= 0; --i) {
        Node* parent = parents[i];
        parent->inc_simulations();
        if (result * flipper == 1)
            parent->inc_wins();
        flipper = -flipper;
    }
}

std::vector<Move> GameTree::pv()
{
    std::vector<Move> pv;
    Node* curr = &root;
    while (!curr->leaf_node()) {
        curr = curr->best_child();
        pv.push_back(curr->get_move());
    }
    return pv;
}

void GameTree::search()
{
    time_ms start_time = utils::curr_time();
    time_ms prev_print_time = start_time;
    bool root_flipped = root.get_position().is_flipped();
    int stm = root_flipped ? 1 : 0;

    while (!stopped()) {
        auto selection = select();
        auto& [curr, parents] = selection;
        int curr_result = curr->get_result();
        if (curr_result == TERMINAL)
        {
            backprop(parents, 1);
        }
        else
        {
            if (curr_result != PENDING)
            {
                log(curr_result);
                curr->display();
            }
            assert(curr_result == PENDING);
            while (!curr->fully_expanded()) {
                curr->expand();
                Node* child = curr->latest_child();

                int result = child->simulate(stm);
                if (result == -1)
                    curr->inc_wins();
                curr->inc_simulations();
                backprop(parents, result);
            }
        }
        time_ms now = utils::curr_time();
        if (now - prev_print_time >= 1000)
        {
            std::vector<Move> pv = this->pv();
            std::cout << "info cp " << root.value_score()
                    << " nodes " << root.get_simulations()
                    << " time " << now - start_time
                    << " nps " << root.get_simulations() * 1000 / (now - start_time)
                    << " pv " << uci::get_pv_string(pv, root_flipped)
                    << std::endl;
            prev_print_time = now;
        }
    }

    Move best_move = root.best_child()->get_move();
    std::cout << "bestmove " << get_move_string(best_move, root_flipped) << std::endl;
}
