/*
 * Teki, a free UCI-compliant chess engine
 * Copyright (C) 2017 Manik Charan
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <random>
#include <algorithm>

#include "controller.h"
#include "evaluate.h"
#include "position.h"
#include "move.h"
#include "utils.h"
#include "uci.h"

constexpr int EQUAL_BOUND = 50;

enum MoveOrder
{
    HASH_MOVE = 300000,
    GOOD_CAP = 290000,
    PROM = 280000,
    KILLER = 270000,
    BAD_CAP = 260000,
};

struct SearchStack
{
    SearchStack()
    {
        mlist.reserve(218);
        orderlist.reserve(218);
        pv.reserve(128);
    }

    int ply;
    std::vector<Move> mlist;
    std::vector<int> orderlist;
    std::vector<Move> pv;
};

inline bool stopped()
{
    return controller.stop_search
        || (controller.time_dependent && utils::curr_time() >= controller.end_time);
}

void reorder_moves(const Position& pos, SearchStack* ss)
{
    std::vector<Move>& mlist = ss->mlist;
    std::vector<int>& orderlist = ss->orderlist;
    orderlist.clear();

    // Fill order vector
    for (int i = 0; i < mlist.size(); ++i) {
        int order = 0;
        Move move = mlist[i];
        if (move & CAPTURE_MASK)
        {
            if (move & ENPASSANT)
            {
                order = GOOD_CAP + piece_value[PAWN].value() + 10 - PAWN;
                goto push_order;
            }

            int cap_val = piece_value[pos.piece_on(to_sq(move))].value();
            int capper_pt = pos.piece_on(from_sq(move));
            int cap_diff = cap_val - piece_value[capper_pt].value();

            if (move & PROM_CAPTURE)
                cap_val += piece_value[prom_type(move)].value();

            if (cap_diff > EQUAL_BOUND)
                order = GOOD_CAP + cap_val - capper_pt;
            else if (cap_diff > -EQUAL_BOUND)
                order = GOOD_CAP + capper_pt;
            else
                order = BAD_CAP - capper_pt;
        }
        else if (move & PROMOTION)
        {
            order = PROM + prom_type(move);
        }
        else
        {
            order = psqt[pos.piece_on(from_sq(move))][to_sq(move)].value();
        }

push_order:
        orderlist.push_back(order);
    }

    assert(mlist.size() == orderlist.size());

    // Sort moves
    for (int i = 1; i < mlist.size(); ++i) {
        int order_to_shift = orderlist[i];
        Move move_to_shift = mlist[i];
        int j;
        for (j = i - 1; j >= 0 && order_to_shift > orderlist[j]; --j) {
            orderlist[j+1] = orderlist[j];
            mlist[j+1] = mlist[j];
        }
        orderlist[j+1] = order_to_shift;
        mlist[j+1] = move_to_shift;
    }

    for (int i = 1; i < orderlist.size(); ++i)
        assert(orderlist[i-1] >= orderlist[i]);
}

int qsearch(Position& pos, SearchStack* const ss, int alpha, int beta)
{
    if (pos.get_half_moves() > 99 || pos.is_repetition())
        return 0;

    if (ss->ply >= MAX_PLY)
        return pos.evaluate();

    // Mate distance pruning
    alpha = std::max((-MATE + ss->ply), alpha);
    beta  = std::min((MATE - ss->ply), beta);
    if (alpha >= beta)
        return alpha;

    bool in_check = pos.in_check(US);
    if (!in_check)
    {
        int eval = pos.evaluate();
        if (eval >= beta)
            return beta;
        if (eval > alpha)
            alpha = eval;
    }

    std::vector<Move>& mlist = ss->mlist;
    mlist.clear();
    pos.generate_quiesce_movelist(mlist);
    reorder_moves(pos, ss);

    int legal_moves = 0;
    for (Move move : mlist) {
        assert((move & CAPTURE_MASK) || (move & PROMOTION));
        Position child_pos = pos;
        if (!child_pos.make_move(move))
            continue;

        ++legal_moves;

        int value = -qsearch(child_pos, ss + 1, -beta, -alpha);

        if (stopped())
            return 0;

        if (value > alpha)
        {
            alpha = value;
            if (value >= beta)
                return beta;
        }
    }

    if (!legal_moves && in_check)
        return -MATE + ss->ply;

    return alpha;
}

int search(Position& pos, SearchStack* const ss, int alpha, int beta, int depth)
{
    ss->pv.clear();
    if (depth <= 0)
        return qsearch(pos, ss, alpha, beta);

    if (ss->ply)
    {
        if (pos.get_half_moves() > 99 || pos.is_repetition())
            return 0;

        if (ss->ply >= MAX_PLY)
            return pos.evaluate();

        // Mate distance pruning
        alpha = std::max((-MATE + ss->ply), alpha);
        beta  = std::min((MATE - ss->ply), beta);
        if (alpha >= beta)
            return alpha;
    }

    std::vector<Move>& mlist = ss->mlist;
    mlist.clear();
    pos.generate_movelist(mlist);
    reorder_moves(pos, ss);

    int best_value = -INFINITY,
        legal_moves = 0;
    Move best_move;
    for (Move move : mlist) {
        Position child_pos = pos;
        if (!child_pos.make_move(move))
            continue;

        ++legal_moves;

        if (!ss->ply)
            uci::print_currmove(move, legal_moves, controller.start_time, pos.is_flipped());

        int value = -search(child_pos, ss + 1, -beta, -alpha, depth - 1);

        if (stopped())
            return 0;

        if (value > best_value)
        {
            best_value = value;
            best_move = move;

            ss->pv.clear();
            ss->pv.push_back(move);
            ss->pv.insert(ss->pv.end(), ss[1].pv.begin(), ss[1].pv.end());

            if (value > alpha)
            {
                alpha = value;
                if (value >= beta)
                    break;
            }
        }
    }

    if (!legal_moves)
        return pos.in_check(US) ? -MATE + ss->ply : 0;

    return best_value;
}

Move Position::best_move()
{
    SearchStack search_stack[MAX_PLY];
    SearchStack* ss = search_stack;
    for (i32 ply = 0; ply < MAX_PLY; ++ply)
        search_stack[ply].ply = ply;

    Move best_move;
    for (int depth = 1; depth < MAX_PLY; ++depth) {
        int score = search(*this, ss, -INFINITY, +INFINITY, depth);

        if (depth > 1 && stopped())
            break;

        time_ms time_passed = utils::curr_time() - controller.start_time;
        uci::print_search(score, depth, 0, time_passed, ss->pv, is_flipped());

        best_move = ss->pv[0];
    }

    return best_move;
}
