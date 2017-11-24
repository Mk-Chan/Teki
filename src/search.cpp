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
#include "tt.h"

constexpr int MAX_HISTORY_DEPTH = 12;
constexpr int HISTORY_LIMIT = 8000;
constexpr int EQUAL_BOUND = 50;

enum MoveOrder
{
    HASH_MOVE = 300000,
    HANGING_CAP = 290000,
    GOOD_CAP = 280000,
    PROM = 270000,
    KILLER = 260000,
    BAD_CAP = 250000,
};

struct SearchStack
{
    SearchStack()
    {
        mlist.reserve(218);
        orderlist.reserve(218);
        pv.reserve(128);
        killer_move[0] = killer_move[1] = 0;
    }

    int ply;
    Move killer_move[2];
    std::vector<Move> mlist;
    std::vector<int> orderlist;
    std::vector<Move> pv;
};

int history[6][64];

inline void reduce_history(bool to_zero=false)
{
    for (int pt = PAWN; pt <= KING; ++pt) {
        for (int sq = 0; sq < 64; ++sq) {
            if (to_zero)
                history[pt][sq] = 0;
            else
                history[pt][sq] /= 8;
        }
    }
}

inline int value_to_tt(int value, int ply)
{
    if (value >= MAX_MATE_VALUE)
        value += ply;
    else if (value <= -MAX_MATE_VALUE)
        value -= ply;
    return value;
}

inline int value_from_tt(int value, int ply)
{
    if (value >= MAX_MATE_VALUE)
        value -= ply;
    else if (value <= -MAX_MATE_VALUE)
        value += ply;
    return value;
}

inline bool stopped()
{
    return controller.stop_search
        || (controller.time_dependent && utils::curr_time() >= controller.end_time);
}

void reorder_moves(const Position& pos, SearchStack* ss, Move tt_move=0)
{
    std::vector<Move>& mlist = ss->mlist;
    std::vector<int>& orderlist = ss->orderlist;
    orderlist.clear();

    // Fill order vector
    for (int i = 0; i < mlist.size(); ++i) {
        int order = 0;
        Move move = mlist[i];
        if (move == tt_move)
        {
            order = HASH_MOVE;
        }
        else if (move & PROMOTION)
        {
            order = PROM + prom_type(move);
        }
        else if (move == ss->killer_move[0])
        {
            order = KILLER;
        }
        else if (move == ss->killer_move[1])
        {
            order = KILLER - 1;
        }
        else if (move & CAPTURE_MASK)
        {
            if (move & ENPASSANT)
            {
                order = GOOD_CAP + 10 + PAWN;
                goto push_order;
            }

            bool defended = pos.attackers_to(to_sq(move), THEM) > 0;
            int cap_val = piece_value[pos.piece_on(to_sq(move))].value();
            int capper_pt = pos.piece_on(from_sq(move));
            if (move & PROM_CAPTURE)
                cap_val += piece_value[prom_type(move)].value();

            if (defended)
            {
                int cap_diff = cap_val - piece_value[capper_pt].value();

                if (cap_diff > EQUAL_BOUND)
                    order = GOOD_CAP + cap_val - capper_pt;
                else if (cap_diff > -EQUAL_BOUND)
                    order = GOOD_CAP + capper_pt;
                else
                    order = BAD_CAP - capper_pt;
            }
            else
            {
                order = HANGING_CAP + cap_val - capper_pt;
            }
        }
        else
        {
            order = history[pos.piece_on(from_sq(move))][to_sq(move)];
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

    if (!(controller.nodes_searched & 2047) && stopped())
        return 0;

    if (ss->ply >= MAX_PLY)
        return pos.evaluate();

    // Mate distance pruning
    alpha = std::max((-MATE + ss->ply), alpha);
    beta  = std::min((MATE - ss->ply), beta);
    if (alpha >= beta)
        return alpha;

    bool in_check = pos.checkers_to(US);
    if (!in_check)
    {
        int eval = pos.evaluate();
        if (eval >= beta)
            return beta;
        if (eval > alpha)
            alpha = eval;
    }

    ++controller.nodes_searched;

    std::vector<Move>& mlist = ss->mlist;
    mlist.clear();
    if (in_check)
        pos.generate_in_check_movelist(mlist);
    else
        pos.generate_quiesce_movelist(mlist);
    reorder_moves(pos, ss);

    int legal_moves = 0;
    for (Move move : mlist) {
        if (!pos.legal_move(move))
            continue;
        Position child_pos = pos;
        child_pos.make_move(move);

        ++legal_moves;

        int value = -qsearch(child_pos, ss + 1, -beta, -alpha);

        if (!(controller.nodes_searched & 2047) && stopped())
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

template <bool pv_node>
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

    if (!(controller.nodes_searched & 2047) && stopped())
        return 0;

    Move tt_move = 0;
    const TTEntry& tt_entry = tt.probe(pos.get_hash_key());
    if (tt_entry.get_key() == pos.get_hash_key())
    {
        tt_move = tt_entry.get_move();
        if (!pv_node && tt_entry.get_depth() >= depth)
        {
            int tt_score = value_from_tt(tt_entry.get_score(), ss->ply);
            int tt_flag = tt_entry.get_flag();
            if (    tt_flag == FLAG_EXACT
                || (tt_flag == FLAG_LOWER && tt_score >= beta)
                || (tt_flag == FLAG_UPPER && tt_score <= alpha))
            {
                return tt_score;
            }
        }
    }

    ++controller.nodes_searched;

    std::vector<Move>& mlist = ss->mlist;
    mlist.clear();

    bool in_check = pos.checkers_to(US);
    if (in_check)
        pos.generate_in_check_movelist(mlist);
    else
        pos.generate_movelist(mlist);
    reorder_moves(pos, ss, tt_move);

    int old_alpha = alpha;
    int best_value = -INFINITY,
        legal_moves = 0;
    Move best_move;
    for (Move move : mlist) {
        if (!pos.legal_move(move))
            continue;
        Position child_pos = pos;
        child_pos.make_move(move);

        ++legal_moves;

        if (!ss->ply)
            uci::print_currmove(move, legal_moves, controller.start_time, pos.is_flipped());

        int depth_left = depth - 1;

        int value;
        if (legal_moves == 1)
        {
            value = -search<pv_node>(child_pos, ss + 1, -beta , -alpha, depth_left);
        }
        else
        {
            value = -search<false>(child_pos, ss + 1, -alpha - 1, -alpha, depth_left);
            if (value > alpha)
                value = -search<pv_node>(child_pos, ss + 1, -beta , -alpha, depth_left);
        }

        if (!(controller.nodes_searched & 2047) && stopped())
            return 0;

        if (value > best_value)
        {
            best_value = value;
            best_move = move;

            if (value > alpha)
            {
                alpha = value;

                if (pv_node)
                {
                    ss->pv.clear();
                    ss->pv.push_back(move);
                    ss->pv.insert(ss->pv.end(), ss[1].pv.begin(), ss[1].pv.end());
                }

                int quiet_move = !((move & CAPTURE_MASK) || (move & PROMOTION));
                if (quiet_move && depth <= MAX_HISTORY_DEPTH)
                {
                    int pt = pos.piece_on(from_sq(move));
                    int to = to_sq(move);
                    history[pt][to] += depth * depth;
                    if (history[pt][to] > HISTORY_LIMIT)
                        reduce_history();
                }

                if (value >= beta)
                {
                    if (quiet_move && move != ss->killer_move[0])
                    {
                        ss->killer_move[1] = ss->killer_move[0];
                        ss->killer_move[0] = move;
                    }
                    break;
                }
            }
        }
    }

    if (!legal_moves)
        return pos.checkers_to(US) ? -MATE + ss->ply : 0;

    u64 flag = best_value >= beta ? FLAG_LOWER
        : best_value > old_alpha ? FLAG_EXACT
        : FLAG_UPPER;

    TTEntry entry(best_move, flag, depth, value_to_tt(best_value, ss->ply),
                  pos.get_hash_key());
    tt.write(entry);

    return best_value;
}

Move Position::best_move()
{
    SearchStack search_stack[MAX_PLY];
    SearchStack* ss = search_stack;

    for (int ply = 0; ply < MAX_PLY; ++ply)
        search_stack[ply].ply = ply;

    reduce_history(true);
    controller.nodes_searched = 0;

    Move best_move;
    for (int depth = 1; depth < MAX_PLY; ++depth) {
        int score = search<true>(*this, ss, -INFINITY, +INFINITY, depth);

        if (depth > 1 && stopped())
            break;

        time_ms time_passed = utils::curr_time() - controller.start_time;
        uci::print_search(score, depth, time_passed, ss->pv, is_flipped());

        best_move = ss->pv[0];
    }

    return best_move;
}
