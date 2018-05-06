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

#include <thread>

#include "syzygy/tbprobe.h"
#include "controller.h"
#include "evaluate.h"
#include "position.h"
#include "options.h"
#include "move.h"
#include "utils.h"
#include "uci.h"
#include "tt.h"

namespace thread
{
    volatile bool stop;
};

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
        nodes_searched = 0;
        tb_hits = 0;
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

    u64 tb_hits;
    u64 nodes_searched;
    int history[6][64];
};

static SearchStack stacks[MAX_THREADS][MAX_PLY];
static SearchGlobals globals[MAX_THREADS];
static std::thread threads[MAX_THREADS];
static std::pair<int, bool> results[MAX_THREADS];

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
    ++sg.nodes_searched;

    if (pos.get_half_moves() > 99 || pos.is_repetition())
        return -options::spins["Contempt"].value;

    if (!(sg.nodes_searched & 2047) && (stopped() || thread::stop))
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
        Position child_pos = pos;
        child_pos.make_move(move);
        if (child_pos.checkers_to(THEM))
            continue;

        ++legal_moves;

        int value = -qsearch(child_pos, ss + 1, sg, -beta, -alpha);

        if (!(sg.nodes_searched & 2047) && (stopped() || thread::stop))
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

    ++sg.nodes_searched;

    if (pos.get_half_moves() > 99 || pos.is_repetition())
        return -options::spins["Contempt"].value;

    if (ss->ply >= MAX_PLY)
        return pos.evaluate();

    // Mate distance pruning
    alpha = std::max((-MATE + ss->ply), alpha);
    beta  = std::min((MATE - ss->ply), beta);
    if (alpha >= beta)
        return alpha;

    // Check if time is left
    if (!(sg.nodes_searched & 2047) && (stopped() || thread::stop))
        return 0;

    // Transposition table probe
    bool tt_hit = false;
    int tt_score = -INFINITY;
    int tt_flag = -1;
    Move tt_move = 0;
    TTEntry tt_entry = tt.probe(pos.get_hash_key());
    if (tt_entry.get_key() == pos.get_hash_key())
    {
        tt_hit = true;
        tt_move = tt_entry.get_move();
        tt_score = value_from_tt(tt_entry.get_score(), ss->ply);
        tt_flag = tt_entry.get_flag();
        if (!pv_node && tt_entry.get_depth() >= depth)
        {
            if (    tt_flag == FLAG_EXACT
                || (tt_flag == FLAG_LOWER && tt_score >= beta)
                || (tt_flag == FLAG_UPPER && tt_score <= alpha))
            {
                return tt_score;
            }
        }
    }

	// Probe EGTB
	// No castling allowed
	// No fifty moves allowed
	if (   TB_LARGEST > 0
        && !pos.get_castling_rights()
        && !pos.get_half_moves()
        && popcnt(pos.occupancy_bb()) <= (int)TB_LARGEST)
    {
        unsigned int wdl = tb_probe_wdl(
                pos.color_bb(US), pos.color_bb(THEM),
                pos.piece_bb(KING), pos.piece_bb(QUEEN),
                pos.piece_bb(ROOK), pos.piece_bb(BISHOP),
                pos.piece_bb(KNIGHT), pos.piece_bb(PAWN), pos.get_ep_sq(),
                true
                );
        if (wdl != TB_RESULT_FAILED)
        {
            ++sg.tb_hits;
            int d = std::min(depth + 6, MAX_PLY - 1);
            TTEntry entry (0, FLAG_EXACT, d, tb_values[wdl], pos.get_hash_key());
            tt.write(entry);
            return tb_values[wdl];
        }
	}

    int num_non_pawns = popcnt(pos.color_bb(US)
            & ~(pos.piece_bb(KING) ^ pos.piece_bb(PAWN)));
    bool in_check = pos.checkers_to(US);

    // Calculate position evaluation as static eval if no tt hit, otherwise
    // try to use the tt score based on the bound
    int static_eval;
    if (!pv_node)
    {
        if (tt_flag == FLAG_EXACT)
        {
            static_eval = tt_score;
        }
        else
        {
            static_eval = pos.evaluate();
            if (tt_hit)
            {
                if (   (static_eval < tt_score && tt_flag == FLAG_LOWER)
                    || (static_eval > tt_score && tt_flag == FLAG_UPPER))
                {
                    static_eval = tt_score;
                }
            }
        }
    }

    // Forward pruning
    if (   !pv_node
        && num_non_pawns
        && !in_check
        && ss->forward_pruning
        && beta > -MAX_MATE_VALUE)
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
            if (!(sg.nodes_searched & 2047) && (stopped() || thread::stop))
                return 0;

            if (val >= beta)
            {
                if (val >= MAX_MATE_VALUE)
                    val = beta;
                return val;
            }
        }
    }

    // Internal iterative deepening
    if (   !tt_move
        && depth >= 5
        && (pv_node || static_eval + 100 >= beta))
    {
        ss->forward_pruning = false;
        search<true>(pos, ss, sg, alpha, beta, depth - 2);
        ss->forward_pruning = true;

        tt_entry = tt.probe(pos.get_hash_key());
        tt_move = tt_entry.get_move();
    }

    // Get a pre-allocated movelist
    std::vector<Move>& mlist = ss->mlist;
    mlist.clear();

    // Populate the movelist
    if (in_check)
        pos.generate_in_check_movelist(mlist);
    else
        pos.generate_movelist(mlist);

    // Reorder the moves
    reorder_moves(pos, ss, sg, tt_move);

    // In-check extension
    if (in_check)
        ++depth;

    int old_alpha = alpha;
    int best_value = -INFINITY,
        legal_moves = 0;
    Move best_move = 0;
    for (Move move : mlist) {
        // Check for legality and make move
        Position child_pos = pos;
        child_pos.make_move(move);
        if (child_pos.checkers_to(THEM))
            continue;

        ++legal_moves;
        int depth_left = depth - 1;

        // Heuristic pruning and reductions
        if (   best_value > -MAX_MATE_VALUE
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
        if (!(sg.nodes_searched & 2047) && (stopped() || thread::stop))
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
        return pos.checkers_to(US)
            ? -MATE + ss->ply
            : -options::spins["Contempt"].value;

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

template <bool main_thread>
int search_root(Position& pos, SearchStack* const ss, SearchGlobals& sg,
                int alpha, int beta, int depth)
{
    ss->pv.clear();
    ++sg.nodes_searched;

    // Check if time is left
    if (!(sg.nodes_searched & 2047) && (stopped() || thread::stop))
        return 0;

    // Transposition table probe
    TTEntry tt_entry = tt.probe(pos.get_hash_key());
    Move tt_move = tt_entry.get_key() == pos.get_hash_key()
        ? tt_entry.get_move()
        : 0;

	// Probe EGTB
	// No castling allowed
	// No fifty moves allowed
	if (   TB_LARGEST > 0
        && !pos.get_castling_rights()
        && !pos.get_half_moves()
        && popcnt(pos.occupancy_bb()) <= (int)TB_LARGEST)
    {
        unsigned int res = tb_probe_root(
                pos.color_bb(US), pos.color_bb(THEM), pos.piece_bb(KING),
                pos.piece_bb(QUEEN), pos.piece_bb(ROOK),
                pos.piece_bb(BISHOP), pos.piece_bb(KNIGHT),
                pos.piece_bb(PAWN), pos.get_half_moves(), pos.get_ep_sq(),
                true, nullptr
                );
        if (res != TB_RESULT_FAILED)
        {
            u32 prom = 0;
            switch (TB_GET_PROMOTES(res)) {
                case TB_PROMOTES_QUEEN:
                    prom = PROM_TO_QUEEN;
                    break;
                case TB_PROMOTES_KNIGHT:
                    prom = PROM_TO_KNIGHT;
                    break;
                case TB_PROMOTES_ROOK:
                    prom = PROM_TO_ROOK;
                    break;
                case TB_PROMOTES_BISHOP:
                    prom = PROM_TO_BISHOP;
                    break;
                default:
                    break;
            }
            int from = TB_GET_FROM(res);
            int to = TB_GET_TO(res);
            Move move = prom
                ? get_move(from, to, PROMOTION, CAP_NONE, prom)
                : get_move(from, to, NORMAL);
            ss->pv.push_back(move);
            controller.stop_search = true;
            return tb_values[TB_GET_WDL(res)];
        }
    }

    // Get a pre-allocated movelist
    std::vector<Move>& mlist = ss->mlist;
    mlist.clear();

    bool in_check = pos.checkers_to(US);
    if (controller.limited_search)
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
    Move best_move = 0;
    for (Move move : mlist) {
        // Check for legality and make move
        Position child_pos = pos;
        child_pos.make_move(move);
        if (child_pos.checkers_to(THEM))
            continue;

        ++legal_moves;

        // Print move being searched at root
        if (main_thread)
        {
            controller.nodes_searched = 0;
            for (int i = 0; i < options::spins["Threads"].value; ++i)
                controller.nodes_searched += globals[i].nodes_searched;
            uci::print_currmove(move, legal_moves, controller.start_time,
                                pos.is_flipped());
        }

        int depth_left = depth - 1;

        // Principal Variation Search (PVS)
        int value;
        if (legal_moves == 1)
        {
            value = -search<true>(child_pos, ss + 1, sg, -beta , -alpha,
                                  depth_left);
        }
        else
        {
            value = -search<false>(child_pos, ss + 1, sg, -alpha - 1, -alpha,
                                   depth_left);
            if (value > alpha)
                value = -search<true>(child_pos, ss + 1, sg, -beta , -alpha,
                                      std::max(depth_left, depth - 1));
        }

        // Check if time is left
        if (!(sg.nodes_searched & 2047) && (stopped() || thread::stop))
            return 0;

        if (value > best_value)
        {
            best_value = value;
            best_move = move;

            // Update PV
            ss->pv.clear();
            ss->pv.push_back(move);
            ss->pv.insert(ss->pv.end(), ss[1].pv.begin(), ss[1].pv.end());

            if (value > alpha)
            {
                alpha = value;

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
        return pos.checkers_to(US)
            ? -MATE + ss->ply
            : -options::spins["Contempt"].value;

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

void parallel_search(Position pos, int alpha, int beta, int depth, int threadnum)
{
    auto& [value, valid] = results[threadnum];
    auto& sg = globals[threadnum];
    SearchStack* ss = stacks[threadnum];
    valid = false; // Mark as invalid result

    // Start parallel search
    value = search_root<false>(pos, ss, sg, alpha, beta, depth);

    // If this thread finished searching before the others, then thread::stop
    // will not be set. Therefore set it to stop all others and mark this
    // result as a valid result
    if (!thread::stop)
    {
        thread::stop = true;
        valid = true;
    }
}

std::pair<Move, Move> Position::best_move()
{
    constexpr int asp_delta[] = { 10, 30, 50, 100, 200, 300, INFINITY };

    int num_threads = options::spins["Threads"].value;

    for (int i = 0; i < num_threads; ++i) {
        // Reset stacks
        for (int ply = 0; ply < MAX_PLY; ++ply)
            stacks[i][ply].ply = ply;

        // Reset results
        results[i].first = 0;
        results[i].second = false;

        // Reset globals
        globals[i].reduce_history(true);
        globals[i].nodes_searched = 0;
        globals[i].tb_hits = 0;
    }

    Move best_move = 0;
    Move ponder_move = 0;
    int alpha = -INFINITY;
    int beta = +INFINITY;
    int adelta;
    int bdelta;
    bool failed;
    int result_index;
    int score = 0;
    for (int depth = 1; depth <= controller.max_ply; ++depth) {
        adelta = bdelta = 0;
        do {
            failed = false;
            thread::stop = false;
            result_index = 0; // Assume main thread has completed search

            // Perform multithreaded search
            if (depth > 4)
            {
                // Start helper threads
                for (int i = 1; i < num_threads; ++i) {
                    threads[i] = std::thread {parallel_search, *this, alpha,
                                              beta, depth + (i & 1), i};
                }

                // Start main thread
                results[0].first = search_root<true>(
                    *this, stacks[0], globals[0], alpha, beta, depth
                );

                // Stop all threads
                thread::stop = true;

                // If no other thread has completed searching, main result will
                // be used since we started by assuming the main has completed.
                // Otherwise, join all threads and use the result of the one
                // with the completed search.
                for (int i = 1; i < num_threads; ++i) {
                    threads[i].join();
                    // If this thread has completed search, use it
                    if (results[i].second)
                        result_index = i;
                }
            }
            // Perform single threaded search
            else
            {
                results[0].first = search_root<true>(
                    *this, stacks[0], globals[0], alpha, beta, depth
                );
            }

            // Get the completed search result
            score = results[result_index].first;

            if (stopped())
                break;

            controller.nodes_searched = 0;
            controller.tb_hits = 0;
            for (int i = 0; i < num_threads; ++i) {
                controller.nodes_searched += globals[i].nodes_searched;
                controller.tb_hits += globals[i].tb_hits;
            }

            SearchStack* ss = stacks[result_index];
            time_ms time_passed = utils::curr_time() - controller.start_time;
            int bound = score >= beta
                           ? LOWER_BOUND
                           : score <= alpha
                               ? UPPER_BOUND
                               : EXACT_BOUND;
            uci::print_search(score, depth, bound, time_passed, ss->pv, is_flipped());

            // Failed low, decrease alpha and repeat
            if (score <= alpha)
            {
                alpha = std::max(score - asp_delta[adelta], -INFINITY);
                ++adelta;
                failed = true;
            }

            // Failed high, increase beta and repeat
            else if (score >= beta)
            {
                beta = std::min(score + asp_delta[bdelta], +INFINITY);
                ++bdelta;
                failed = true;
            }
        }
        while (failed);

        if (depth > 1 && stopped())
            break;

        SearchStack* ss = stacks[result_index];
        best_move = ss->pv[0];
        ponder_move = 0;
        if (depth > 1 && ss->pv.size() > 1)
            ponder_move = ss->pv[1];

        // Prepare aspiration window for next time
        if (depth > 4)
        {
            alpha = score - 10;
            beta = score + 10;
        }
    }

    // Do not print bestmove during go infinite or ponder
    while (controller.analyzing)
        continue;

    return { best_move, ponder_move };
}
