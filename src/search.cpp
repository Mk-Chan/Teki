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
        forward_pruning = true;
        mlist.reserve(218);
        orderlist.reserve(218);
        pv.reserve(128);
        killer_move[0] = killer_move[1] = 0;
    }

    int ply;
    bool forward_pruning;
    Move killer_move[2];
    std::vector<Move> mlist;
    std::vector<int> orderlist;
    std::vector<Move> pv;
};

struct SearchGlobals
{
    SearchGlobals()
    {
        reduce_history(true);
    }

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

    int history[6][64];
};

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
        || (   controller.time_dependent
            && utils::curr_time() >= controller.end_time);
}

void reorder_moves(const Position& pos, SearchStack* ss, SearchGlobals& sg,
        Move tt_move=0)
{
    std::vector<Move>& mlist = ss->mlist;
    std::vector<int>& orderlist = ss->orderlist;
    orderlist.clear();

    // Fill order vector
    for (unsigned i = 0; i < mlist.size(); ++i) {
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
            order = sg.history[pos.piece_on(from_sq(move))][to_sq(move)];
        }

push_order:
        orderlist.push_back(order);

        // Sort moves using insertion sort
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

    assert(mlist.size() == orderlist.size());
}

int qsearch(Position& pos, SearchStack* const ss, SearchGlobals& sg,
            int alpha, int beta)
{
    ++controller.nodes_searched;

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

    std::vector<Move>& mlist = ss->mlist;
    mlist.clear();
    if (in_check)
        pos.generate_in_check_movelist(mlist);
    else
        pos.generate_quiesce_movelist(mlist);
    reorder_moves(pos, ss, sg);

    int legal_moves = 0;
    for (Move move : mlist) {
        if (!pos.legal_move(move))
            continue;
        Position child_pos = pos;
        child_pos.make_move(move);

        ++legal_moves;

        int value = -qsearch(child_pos, ss + 1, sg, -beta, -alpha);

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
int search(Position& pos, SearchStack* const ss, SearchGlobals& sg,
           int alpha, int beta, int depth)
{
    ss->pv.clear();
    if (depth <= 0)
        return qsearch(pos, ss, sg, alpha, beta);

    ++controller.nodes_searched;

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

    // Check if time is left
    if (!(controller.nodes_searched & 2047) && stopped())
        return 0;

    // Transposition table probe
    Move tt_move = 0;
    TTEntry tt_entry = tt.probe(pos.get_hash_key());
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

    int num_non_pawns = popcnt(pos.color_bb(US)
            & ~(pos.piece_bb(KING) ^ pos.piece_bb(PAWN)));
    bool in_check = pos.checkers_to(US);

    int static_eval;
    if (!pv_node)
        static_eval = pos.evaluate();

    // Forward pruning
    if (   !pv_node
        && num_non_pawns
        && !in_check
        && ss->forward_pruning)
    {
        // Reverse futility pruning
        if (   depth < 3
            && static_eval - 200 * depth >= beta)
            return static_eval;

        // Null move pruning (NMP)
        if (depth >= 4 && static_eval >= beta - 100)
        {
            int reduction = 4;
            int depth_left = std::max(1, depth - reduction);
            ss[1].forward_pruning = false;
            Position child = pos;
            child.make_null_move();
            int val = -search<false>(child, ss + 1, sg, -beta, -beta + 1,
                    depth_left);
            ss[1].forward_pruning = true;

            // Check if time is left
            if (!(controller.nodes_searched & 2047) && stopped())
                return 0;

            if (val >= beta)
            {
                if (val >= MAX_MATE_VALUE)
                    val = beta;
                return val;
            }
        }
    }

    // Get a pre-allocated movelist
    std::vector<Move>& mlist = ss->mlist;
    mlist.clear();

    if (controller.limited_search && !ss->ply)
    {
        std::copy(controller.search_moves.begin(),
                  controller.search_moves.end(),
                  std::back_inserter(mlist));
    }
    else
    {
        // Populate the movelist
        if (in_check)
            pos.generate_in_check_movelist(mlist);
        else
            pos.generate_movelist(mlist);
    }

    // Reorder the moves
    reorder_moves(pos, ss, sg, tt_move);

    // In-check extension
    if (in_check)
        ++depth;

    int old_alpha = alpha;
    int best_value = -INFINITY,
        legal_moves = 0;
    Move best_move;
    for (Move move : mlist) {
        // Check for legality and make move
        if (!pos.legal_move(move))
            continue;
        Position child_pos = pos;
        child_pos.make_move(move);

        ++legal_moves;

        // Print move being searched at root
        if (!ss->ply)
            uci::print_currmove(move, legal_moves, controller.start_time,
                    pos.is_flipped());

        int depth_left = depth - 1;

        // Heuristic pruning and reductions
        if (   ss->ply
            && best_value > -MAX_MATE_VALUE
            && legal_moves > 1
            && num_non_pawns
            && !prom_type(move)
            && !cap_type(move)
            && !child_pos.checkers_to(US))
        {
            // Futility pruning
            if (   depth < 8
                && !pv_node
                && static_eval + 100 * depth_left <= alpha)
                continue;

            // Late move reduction (LMR)
            if (   depth > 2
                && legal_moves > (pv_node ? 5 : 3)
                && move != ss->killer_move[0]
                && move != ss->killer_move[1]
                && !in_check
                && !pos.is_passed_pawn(from_sq(move)))
            {
                depth_left -= 1 + !pv_node + (legal_moves > 10);
                depth_left = std::max(1, depth_left);
            }
        }

        // Principal Variation Search (PVS)
        int value;
        if (legal_moves == 1)
        {
            value = -search<pv_node>(child_pos, ss + 1, sg, -beta , -alpha,
                    depth_left);
        }
        else
        {
            value = -search<false>(child_pos, ss + 1, sg, -alpha - 1, -alpha,
                    depth_left);
            if (value > alpha)
                value = -search<pv_node>(child_pos, ss + 1, sg, -beta , -alpha,
                        std::max(depth_left, depth - 1));
        }

        // Check if time is left
        if (!(controller.nodes_searched & 2047) && stopped())
            return 0;

        if (value > best_value)
        {
            best_value = value;
            best_move = move;

            if (value > alpha)
            {
                alpha = value;

                // Update PV
                if (pv_node)
                {
                    ss->pv.clear();
                    ss->pv.push_back(move);
                    ss->pv.insert(ss->pv.end(), ss[1].pv.begin(), ss[1].pv.end());
                }

                // Update history
                bool quiet_move = !((move & CAPTURE_MASK) || (move & PROMOTION));
                if (quiet_move && depth <= MAX_HISTORY_DEPTH)
                {
                    int pt = pos.piece_on(from_sq(move));
                    int to = to_sq(move);
                    sg.history[pt][to] += depth * depth;

                    // Reduce history if it overflows
                    if (sg.history[pt][to] > HISTORY_LIMIT)
                        sg.reduce_history();
                }

                if (value >= beta)
                {
                    // Update killer moves
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

    // Check for checkmate or stalemate
    if (!legal_moves)
        return pos.checkers_to(US) ? -MATE + ss->ply : 0;

    // Transposition entry flag
    u64 flag = best_value >= beta ? FLAG_LOWER
        : best_value > old_alpha ? FLAG_EXACT
        : FLAG_UPPER;

    // Create a tt entry and store it
    TTEntry entry(best_move, flag, depth, value_to_tt(best_value, ss->ply),
                  pos.get_hash_key());
    tt.write(entry);

    return best_value;
}

Move Position::best_move()
{
    SearchGlobals sg;
    SearchStack search_stack[MAX_PLY];
    SearchStack* ss = search_stack;

    for (int ply = 0; ply < MAX_PLY; ++ply)
        search_stack[ply].ply = ply;

    controller.nodes_searched = 0;

    Move best_move;
    for (int depth = 1; depth < MAX_PLY; ++depth) {
        int score = search<true>(*this, ss, sg, -INFINITY, +INFINITY, depth);

        if (depth > 1 && stopped())
            break;

        time_ms time_passed = utils::curr_time() - controller.start_time;
        uci::print_search(score, depth, time_passed, ss->pv, is_flipped());

        best_move = ss->pv[0];
    }

    return best_move;
}
