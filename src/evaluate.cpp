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

#include "position.h"
#include "utils.h"
#include "evaluate.h"

int piece_phase[5] = { 1, 10, 10, 20, 40 };

Score piece_value[5] = {
    S(100, 100), S(400, 300), S(400, 300), S(600, 500), S(1200, 900)
};

Score psqt[6][64];
Score psq_tmp[6][32] = {
    {	// Pawn
        S(  0,   0), S(  0,   0), S(  0,   0), S(  0,   0),
        S(  0,   0), S(  0,   0), S(  0,   0), S(  0,   0),
        S(  0,   5), S(  0,   5), S(  0,   5), S( 10,   5),
        S(  0,  10), S(  0,  10), S(  0,  10), S( 20,  10),
        S(  0,  30), S(  0,  30), S(  0,  30), S( 10,  30),
        S(  0,  50), S(  0,  50), S(  0,  50), S(  0,  50),
        S(  0,  80), S(  0,  80), S(  0,  80), S(  0,  80),
        S(  0,   0), S(  0,   0), S(  0,   0), S(  0,   0)
    },
    {	// Knight
        S(-50, -30), S(-30, -20), S(-20, -10), S(-15,   0),
        S(-30, -20), S( -5, -10), S(  0,  -5), S(  5,   5),
        S(-10, -10), S(  0,  -5), S(  5,   5), S( 10,  10),
        S(-10,   0), S(  5,   0), S( 10,  15), S( 20,  20),
        S(-10,   0), S(  5,   0), S( 10,  15), S( 20,  20),
        S(-10, -10), S(  0,  -5), S(  5,   5), S( 10,  10),
        S(-30, -20), S( -5, -10), S(  0,  -5), S(  5,   5),
        S(-50, -30), S(-30, -20), S(-20, -10), S(-15,   0)
    },
    {	// Bishop
        S(-20, -20), S(-20, -15), S(-20, -10), S(-20, -10),
        S(-10,   0), S(  0,   0), S( -5,   0), S(  0,   0),
        S( -5,   0), S(  5,   0), S(  5,   0), S(  5,   0),
        S(  0,   0), S(  5,   0), S( 10,   0), S( 15,   0),
        S(  0,   0), S(  5,   0), S( 10,   0), S( 15,   0),
        S( -5,   0), S(  5,   0), S(  5,   0), S(  5,   0),
        S(-10,   0), S(  0,   0), S( -5,   0), S(  0,   0),
        S(-20, -20), S(-20, -15), S(-20, -10), S(-20, -10)
    },
    {	// Rook
        S( -5,  -5), S(  0,  -3), S(  2,  -1), S(  5,   0),
        S( -5,   0), S(  0,   0), S(  2,   0), S(  5,   0),
        S( -5,   0), S(  0,   0), S(  2,   0), S(  5,   0),
        S( -5,   0), S(  0,   0), S(  2,   0), S(  5,   0),
        S( -5,   0), S(  0,   0), S(  2,   0), S(  5,   0),
        S( -5,   0), S(  0,   0), S(  2,   0), S(  5,   0),
        S( -5,   0), S(  0,   0), S(  2,   0), S(  5,   0),
        S( -5,  -5), S(  0,  -3), S(  2,  -1), S(  5,   0)
    },
    {
        // Queen
        S(-10, -20), S( -5, -10), S( -5,  -5), S( -5,   0),
        S( -5, -10), S(  0,  -5), S(  0,   0), S(  0,   5),
        S( -5,  -5), S(  0,   5), S(  0,   5), S(  0,  10),
        S( -5,   0), S(  0,   5), S(  0,  10), S(  0,  15),
        S( -5,   0), S(  0,   5), S(  0,  10), S(  0,  15),
        S( -5,  -5), S(  0,   5), S(  0,   5), S(  0,  10),
        S( -5, -10), S(  0,  -5), S(  0,   0), S(  0,   5),
        S(-10, -20), S( -5, -10), S( -5,  -5), S( -5,   0)
    },
    {
        // King
        S( 30, -70), S( 45, -45), S( 35, -35), S(-10, -20),
        S( 10, -40), S( 20, -25), S(  0, -10), S(-15,   5),
        S(-20, -30), S(-25, -15), S(-30,   5), S(-30,  10),
        S(-40, -20), S(-50,   5), S(-60,  10), S(-70,  20),
        S(-70, -20), S(-80,   5), S(-90,  10), S(-90,  20),
        S(-70, -30), S(-80, -15), S(-90,   5), S(-90,  10),
        S(-80, -40), S(-80, -25), S(-90, -10), S(-90,   0),
        S(-90, -70), S(-90, -45), S(-90, -15), S(-90, -20)
    }
};

Score passed_pawn[7] = {
    0, S(5, 5), S(10, 10), S(15, 20), S(30, 40), S(50, 70), S(80, 120)
};
Score doubled_pawns = S(-10, -10);
Score isolated_pawn = S(-10, -10);
Score rook_on_7th_rank = S(40, 20);
Score bishop_pair = S(50, 80);

namespace eval
{
    void init()
    {
        int k = 0;
        for (int i = 0; i < 8; ++i) {
            for (int j = 0;  j < 4; ++j) {
                int sq1 = get_sq(i, j);
                int sq2 = get_sq(i, (7 - j));
                for (int pt = PAWN; pt <= KING; ++pt) {
                    psqt[pt][sq1]
                        = psqt[pt][sq2]
                        = psq_tmp[pt][k];
                }
                ++k;
            }
        }
    }
}

int get_game_phase(const Position& pos)
{
    int phase = 0;
    for (int pt = PAWN; pt < KING; ++pt)
        phase += piece_phase[pt] * popcnt(pos.piece_bb(pt));
    return phase;
}

Score eval_piece_values(const Position& pos)
{
    Score value;
    for (int pt = PAWN; pt < KING; ++pt)
        value += piece_value[pt] * popcnt(pos.piece_bb(pt, US));
    return value;
}

Score eval_psqt_values(const Position& pos)
{
    Score value;
    for (int pt = PAWN; pt <= KING; ++pt) {
        u64 bb = pos.piece_bb(pt, US);
        while (bb) {
            value += psqt[pt][fbitscan(bb)];
            bb &= bb - 1;
        }
    }
    return value;
}

Score eval_pawn_structure(const Position& pos)
{
    Score value;
    u64 all_pawns_bb = pos.piece_bb(PAWN);
    u64 pawn_bb = pos.piece_bb(PAWN, US);
    u64 bb = pawn_bb;
    while (bb) {
        int sq = fbitscan(bb);
        bb &= bb - 1;

        if (lookups::north(sq) & pawn_bb)
            value += doubled_pawns;

        if (!(lookups::adjacent_files(sq) & pawn_bb))
            value += isolated_pawn;

        if (!(lookups::adjacent_forward(sq) & all_pawns_bb))
            value += passed_pawn[rank_of(sq)];
    }
    return value;
}

Score eval_piece_placement(const Position& pos)
{
    Score value;

    if (popcnt(pos.piece_bb(BISHOP, US)) >= 2)
        value += bishop_pair;

    u64 bb = pos.piece_bb(ROOK, US);
    while (bb) {
        int sq = fbitscan(bb);
        bb &= bb - 1;

        if (rank_of(sq) == RANK_7)
            value += rook_on_7th_rank;
    }

    return value;
}

Score eval_piece_mobility(const Position& pos)
{
    Score value;
    u64 their_pawn_atks_bb = 0;
    their_pawn_atks_bb |= ((pos.piece_bb(PAWN, THEM) & ~FILE_A_MASK) >> 9);
    their_pawn_atks_bb |= ((pos.piece_bb(PAWN, THEM) & ~FILE_H_MASK) >> 7);

    for (int pt = KNIGHT; pt < KING; ++pt) {
        u64 bb = pos.piece_bb(pt, US);
        while (bb) {
            int sq = fbitscan(bb);
            bb &= bb - 1;

            value += 5 * popcnt(lookups::attacks(pt, sq, pos.occupancy_bb())
                                & ~their_pawn_atks_bb);
        }
    }

    return value;
}

int Position::evaluate()
{
    Score eval = 0;
    for (i32 side = US; side <= THEM; ++side) {
        eval += eval_piece_values(*this);
        eval += eval_psqt_values(*this);
        eval += eval_pawn_structure(*this);
        eval += eval_piece_placement(*this);
        eval += eval_piece_mobility(*this);

        this->flip();
        eval = -eval;
    }

    int phase = get_game_phase(*this);
    return eval.value(phase, MAX_PHASE);
}
