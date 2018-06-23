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

bool Node::not_expanded()
{
    return !expanded;
}

Node& Node::expand()
{
    if (!expanded) {
        if (mlist.empty())
            generate_children();
        expanded = true;
    }
    Position p {pos};
    Move move = next_move();
    p.make_move(move);
    children.emplace_back(p);
    Node& latest_child = children.back();
    latest_child.set_move(move);
    return latest_child;
}

template <bool pv>
double Node::score(u64 total_simulations)
{
    if constexpr (pv)
    {
        return double(wins) / double(simulations);
    }
    else
    {
        return double(wins) / double(simulations) +
               std::sqrt(2 * std::log(double(total_simulations)) / double(simulations));
    }
}

template <bool pv>
Node& Node::next_child(u64 total_simulations)
{
    int best_index = 0;
    double best_score = children[best_index].score<pv>(total_simulations);
    for (std::size_t i = 1; i < children.size(); ++i) {
        double score = children[i].score<pv>(total_simulations);
        if (score > best_score)
        {
            best_score = score;
            best_index = i;
        }
    }
    return children[best_index];
}

std::reference_wrapper<Node> GameTree::select(Node& root, std::vector<std::reference_wrapper<Node>>& parents)
{
    parents.clear();
    auto curr = std::ref(root);
    while (true) {
        if (curr.get().not_expanded())
            return curr;

        parents.push_back(curr);
        curr = curr.get().next_child<false>(root.get_simulations());
    }
}

void backprop(std::vector<std::reference_wrapper<Node>>& parents, int result)
{
    int flipper = 1;
    for (int i = parents.size()-1; i >= 0; --i) {
        auto parent = parents[i];
        parent.get().inc_simulations();
        if (result * flipper == 1)
            parent.get().inc_wins();
        flipper = -flipper;
    }
}

std::vector<Move> GameTree::pv()
{
    std::vector<Move> pv;
    auto curr = std::ref(root);
    while (curr.get().fully_expanded()) {
        curr = curr.get().next_child<true>(0);
        pv.push_back(curr.get().get_move());
    }
    return pv;
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
    while (true) {
        if (pos.is_drawn())
            return 0;

        // Generate moves for position
        std::vector<Move>& temp_mlist = local_mlist;
        temp_mlist.clear();
        pos.generate_legal_movelist(temp_mlist);
        if (temp_mlist.empty())
            break;

        // Play a random move
        int r = utils::rand_int(0, temp_mlist.size() - 1);
        Move& move = temp_mlist[r];
        pos.make_move(move);
        terminal_stm = !terminal_stm;
        first = false;
        ++depth;
    }

    // Return 0 for a draw
    int result;
    if (!pos.checkers_to(US))
    {
        result = 0;
    }
    else
    {
        int curr_stm = this->pos.is_flipped() ? 1 : 0;
        if ((terminal_stm == original_stm) ^ (curr_stm == original_stm))
        {
            result = 1;
            wins += 1;
        }
        else
        {
            result = -1;
        }
    }

    if (first)
        this->result = result;
    return result;
}

void GameTree::search()
{
    std::vector<std::reference_wrapper<Node>> parents;
    time_ms start_time = utils::curr_time();
    time_ms prev_print_time = start_time;
    bool root_flipped = root.get_position().is_flipped();
    int stm = root_flipped ? 1 : 0;

    while (!stopped()) {
        auto curr = select(root, parents);
        int curr_result = curr.get().get_result();
        if (curr_result != -2)
        {
            backprop(parents, curr_result);
        }
        else
        {
            while (!curr.get().fully_expanded()) {
                Node& child = curr.get().expand();

                int result = child.simulate(stm);
                if (result == -1)
                {
                    curr.get().inc_wins();
                }
                curr.get().inc_simulations();
                backprop(parents, result);
            }
        }
        time_ms now = utils::curr_time();
        if (now - prev_print_time >= 1000)
        {
            std::vector<Move> pv = this->pv();
            std::cout << "info cp " << root.score<true>(0)
                    << " nodes " << root.get_simulations()
                    << " time " << now - start_time
                    << " nps " << root.get_simulations() * 1000 / (now - start_time)
                    << " pv " << uci::get_pv_string(pv, root_flipped)
                    << std::endl;
            prev_print_time = now;
        }
    }

    Move best_move = root.next_child<true>(0).get_move();
    std::cout << "bestmove " << get_move_string(best_move, root_flipped) << std::endl;
}