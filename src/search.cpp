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

#include "time_manager.h"
#include "position.h"
#include "move.h"
#include "utils.h"
#include "uci.h"

struct SearchStack
{
    SearchStack()
    {
        mlist.reserve(218);
        pv.reserve(128);
    }

    int ply;
    std::vector<Move> mlist;
    std::vector<Move> pv;
};

inline bool stopped()
{
    return time_manager.time_dependent && utils::curr_time() >= time_manager.end_time;
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

    int legal_moves = 0;
    for (Move move : mlist) {
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

    int best_value = -INFINITY,
        legal_moves = 0;
    Move best_move;
    for (Move move : mlist) {
        Position child_pos = pos;
        if (!child_pos.make_move(move))
            continue;

        ++legal_moves;

        if (!ss->ply)
            uci::print_currmove(move, legal_moves, time_manager.start_time, pos.is_flipped());

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

        time_ms time_passed = utils::curr_time() - time_manager.start_time;
        uci::print_search(score, depth, 0, time_passed, ss->pv, is_flipped());

        best_move = ss->pv[0];
    }

    return best_move;
}
