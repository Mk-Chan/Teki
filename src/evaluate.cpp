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

enum PassedPawnType
{
    CANNOT_ADVANCE,
    UNSAFE_ADVANCE,
    PROTECTED_ADVANCE,
    SAFE_ADVANCE
};

int piece_phase[5] = { 1, 10, 10, 20, 40 };

Score piece_value[5] = {
    S(100, 100), S(400, 300), S(400, 300), S(600, 500), S(1200, 900)
};

Score psqt[6][64];
Score psq_tmp[6][32] = {
    {   // Pawn
        S(  0,   0), S(  0,   0), S(  0,   0), S(  0,   0),
        S(  0,   0), S(  0,   0), S(  0,   0), S(  0,   0),
        S(  0,   5), S(  0,   5), S(  0,   5), S( 10,   5),
        S(  0,  10), S(  0,  10), S(  0,  10), S( 20,  10),
        S(  0,  30), S(  0,  30), S(  0,  30), S( 10,  30),
        S(  0,  50), S(  0,  50), S(  0,  50), S(  0,  50),
        S(  0,  80), S(  0,  80), S(  0,  80), S(  0,  80),
        S(  0,   0), S(  0,   0), S(  0,   0), S(  0,   0)
    },
    {   // Knight
        S(-50, -30), S(-30, -20), S(-20, -10), S(-15,   0),
        S(-30, -20), S( -5, -10), S(  0,  -5), S(  5,   5),
        S(-10, -10), S(  0,  -5), S(  5,   5), S( 10,  10),
        S(-10,   0), S(  5,   0), S( 10,  15), S( 20,  20),
        S(-10,   0), S(  5,   0), S( 10,  15), S( 20,  20),
        S(-10, -10), S(  0,  -5), S(  5,   5), S( 10,  10),
        S(-30, -20), S( -5, -10), S(  0,  -5), S(  5,   5),
        S(-50, -30), S(-30, -20), S(-20, -10), S(-15,   0)
    },
    {   // Bishop
        S(-20, -20), S(-20, -15), S(-20, -10), S(-20, -10),
        S(-10,   0), S(  0,   0), S( -5,   0), S(  0,   0),
        S( -5,   0), S(  5,   0), S(  5,   0), S(  5,   0),
        S(  0,   0), S(  5,   0), S( 10,   0), S( 15,   0),
        S(  0,   0), S(  5,   0), S( 10,   0), S( 15,   0),
        S( -5,   0), S(  5,   0), S(  5,   0), S(  5,   0),
        S(-10,   0), S(  0,   0), S( -5,   0), S(  0,   0),
        S(-20, -20), S(-20, -15), S(-20, -10), S(-20, -10)
    },
    {   // Rook
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

Score passed_pawn[4][7] = {
    { 0, S(4, 4), S(8, 8), S(12, 18), S(27, 35), S(75, 110), S(100, 220) },
    { 0, S(5, 5), S(10, 10), S(15, 20), S(30, 40), S(80, 120), S(120, 250) },
    { 0, S(7, 7), S(12, 12), S(17, 22), S(35, 45), S(90, 160), S(130, 300) },
    { 0, S(7, 7), S(15, 15), S(20, 25), S(40, 50), S(100, 200), S(150, 400) }
};
Score doubled_pawns = S(-10, -10);
Score isolated_pawn = S(-10, -10);

Score rook_on_7th_rank = S(40, 20);
Score bishop_pair = S(50, 80);

int king_attack_weight[5] = { 0, 3, 3, 4, 5 };
int king_attack_table[100] = { // Taken from CPW(Glaurung 1.2)
      0,   0,   0,   1,   1,   2,   3,   4,   5,   6,
      8,  10,  13,  16,  20,  25,  30,  36,  42,  48,
     55,  62,  70,  80,  90, 100, 110, 120, 130, 140,
    150, 160, 170, 180, 190, 200, 210, 220, 230, 240,
    250, 260, 270, 280, 290, 300, 310, 320, 330, 340,
    350, 360, 370, 380, 390, 400, 410, 420, 430, 440,
    450, 460, 470, 480, 490, 500, 510, 520, 530, 540,
    550, 560, 570, 580, 590, 600, 610, 620, 630, 640,
    650, 650, 650, 650, 650, 650, 650, 650, 650, 650,
    650, 650, 650, 650, 650, 650, 650, 650, 650, 650
};

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

struct Evaluator
{
    Evaluator(Position&);
    int evaluate();

private:
    int get_game_phase();
    Score eval_pawns();
    Score eval_pieces();
    Score eval_passed_pawns();
    Score eval_king();

    // Data members
    int side;
    int king_attacks[2];
    u64 passed_pawn_bb[2];
    u64 attacked_by[2][8];
    Position& pos;
};

Evaluator::Evaluator(Position& pos) : pos(pos) {}

int Evaluator::get_game_phase()
{
    int phase = 0;
    for (int pt = PAWN; pt < KING; ++pt)
        phase += piece_phase[pt] * popcnt(pos.piece_bb(pt));
    return phase;
}

Score Evaluator::eval_pawns()
{
    Score value;
    u64 all_pawns_bb = pos.piece_bb(PAWN);
    u64 pawn_bb = pos.piece_bb(PAWN, US);

    // Material value
    value += piece_value[PAWN] * popcnt(pawn_bb);

    u64 bb = pawn_bb;
    while (bb) {
        int sq = fbitscan(bb);
        bb &= bb - 1;

        // Piece square value
        value += psqt[PAWN][sq];

        // Doubled pawn
        if (lookups::north(sq) & pawn_bb)
            value += doubled_pawns;

        // Isolated Pawn
        if (!(lookups::adjacent_files(sq) & pawn_bb))
            value += isolated_pawn;

        // Passed pawn
        if (!(lookups::passed_pawn_mask(sq) & all_pawns_bb))
            this->passed_pawn_bb[this->side] ^= BB(sq);
    }
    return value;
}

Score Evaluator::eval_pieces()
{
    Score value;

    // Squares covered by their pawns' attacks
    u64 their_atks_bb = 0;
    their_atks_bb |= ((pos.piece_bb(PAWN, THEM) & ~FILE_A_MASK) >> 9);
    their_atks_bb |= ((pos.piece_bb(PAWN, THEM) & ~FILE_H_MASK) >> 7);

    // Update pawn attacks
    this->attacked_by[this->side][PAWN] |= their_atks_bb;
    this->attacked_by[this->side][ALL_PIECES] |= their_atks_bb;

    // Bishop pair
    if (popcnt(pos.piece_bb(BISHOP, US)) >= 2)
        value += bishop_pair;

    // Rook on relative 7th rank
    value += rook_on_7th_rank * popcnt(pos.piece_bb(ROOK, US) & RANK_7_MASK);

    for (int pt = KNIGHT; pt < KING; ++pt) {
        u64 bb = pos.piece_bb(pt, US);

        // Material value
        value += piece_value[pt] * popcnt(bb);

        while (bb) {
            int sq = fbitscan(bb);
            bb &= bb - 1;

            // Piece square value
            value += psqt[pt][sq];

            // Mobility
            u64 atks_bb = lookups::attacks(pt, sq, pos.occupancy_bb());
            value += 5 * popcnt(atks_bb & ~their_atks_bb);

            // Update attacks
            this->attacked_by[this->side][pt] |= atks_bb;
            this->attacked_by[this->side][ALL_PIECES] |= atks_bb;

            // Attacks to their king
            u64 king_atks_bb = atks_bb & lookups::king_danger_zone(pos.position_of(KING, THEM));
            if (king_atks_bb)
            {
                this->king_attacks[this->side] +=
                    king_attack_weight[pt] * popcnt(king_atks_bb);
            }
        }
    }

    return value;
}

Score Evaluator::eval_king()
{
    Score value;

    int ksq = pos.position_of(KING, US);

    // Update attacks
    u64 atks_bb = lookups::king(ksq);
    this->attacked_by[this->side][KING] |= atks_bb;
    this->attacked_by[this->side][ALL_PIECES] |= atks_bb;

    // Piece square value
    value += psqt[KING][ksq];

    // Lookup king attack index
    int val = king_attack_table[std::min(this->king_attacks[this->side], 99)];
    value += S(val, val/2);
    return value;
}

Score Evaluator::eval_passed_pawns()
{
    Score value;
    u64 occupancy = pos.occupancy_bb();
    u64 passed_pawn_bb = this->passed_pawn_bb[this->side];
    while (passed_pawn_bb) {
        int sq = fbitscan(passed_pawn_bb);
        passed_pawn_bb &= passed_pawn_bb - 1;

        int rank = rank_of(sq);
        u64 forward = BB(sq + 8);

        // Passed pawn value
        if (forward & pos.occupancy_bb())
        {
            value += passed_pawn[CANNOT_ADVANCE][rank];
        }
        else
        {
            u64 path_to_queen = lookups::north(sq),
                defended = path_to_queen,
                attacked = path_to_queen;

            u64 xrayers = (pos.piece_bb(ROOK) | pos.piece_bb(QUEEN))
                         & lookups::south(sq)
                         & lookups::rook(sq, occupancy);

            if (!(xrayers & pos.color_bb(US)))
                defended &= attacked_by[this->side][ALL_PIECES];
            if (!(xrayers & pos.color_bb(THEM)))
                attacked &= attacked_by[!this->side][ALL_PIECES];

            int type;
            if (!attacked)
                type = SAFE_ADVANCE;
            else if (attacked && !defended)
                type = UNSAFE_ADVANCE;
            else
                type = PROTECTED_ADVANCE;

            value += passed_pawn[type][rank];
        }
    }
    return value;
}

int Evaluator::evaluate()
{
    Score score;
    for (i32 side = US; side <= THEM; ++side) {
        this->side = side;
        this->king_attacks[this->side] = 0;
        this->passed_pawn_bb[this->side] = 0;
        this->attacked_by[this->side][ALL_PIECES] = 0;
        for (int pt = PAWN; pt <= KING; ++pt)
            this->attacked_by[this->side][pt] = 0;

        score += eval_pawns();
        score += eval_pieces();

        pos.flip();
        score = -score;
    }

    for (i32 side = US; side <= THEM; ++side) {
        this->side = side;

        score += eval_king();
        score += eval_passed_pawns();

        pos.flip();
        score = -score;
    }

    int phase = get_game_phase();
    return score.value(phase, MAX_PHASE);
}

int Position::evaluate()
{
    Evaluator evaluator(*this);
    return evaluator.evaluate();
}
