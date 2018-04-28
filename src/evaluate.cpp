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
    u64 blocked_pawn_bb[2];
    u64 passed_pawn_bb[2];
    u64 attacked_by[2][8];
    Position& pos;
};

Evaluator::Evaluator(Position& pos) : pos(pos)
{
    king_attacks[US] = king_attacks[THEM] = 0;
    blocked_pawn_bb[US] = blocked_pawn_bb[THEM] = 0;
    passed_pawn_bb[US] = passed_pawn_bb[THEM] = 0;
    attacked_by[US][ALL_PIECES] = attacked_by[THEM][ALL_PIECES] = 0;
    for (int pt = PAWN; pt <= KING; ++pt)
        attacked_by[US][pt] = attacked_by[THEM][pt] = 0;
}

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
    u64 their_pawns_bb = pos.piece_bb(PAWN, THEM);
    u64 pawn_bb = pos.piece_bb(PAWN, US);

    // Store blocked pawns
    this->blocked_pawn_bb[this->side] = ((pawn_bb << 8) & their_pawns_bb) >> 8;

    // Squares covered by our pawns' attacks
    u64 attacked = 0;
    attacked |= ((pawn_bb & ~FILE_A_MASK) << 7);
    attacked |= ((pawn_bb & ~FILE_H_MASK) << 9);

    // Update pawn attacks
    this->attacked_by[this->side][PAWN] |= attacked;
    this->attacked_by[this->side][ALL_PIECES] |= attacked;

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

        // Passed pawn
        else if (!(lookups::passed_pawn_mask(sq) & their_pawns_bb))
            this->passed_pawn_bb[this->side] ^= BB(sq);

        // Isolated Pawn
        if (!(lookups::adjacent_files(sq) & pawn_bb))
            value += isolated_pawn;
    }
    return value;
}

Score Evaluator::eval_pieces()
{
    Score value;

    // Squares which are occupied by our pawns or king or defended by the
    // opponent's pawns
    u64 mobility_mask = 0;
    mobility_mask |= ((pos.piece_bb(PAWN, THEM) & ~FILE_A_MASK) >> 9);
    mobility_mask |= ((pos.piece_bb(PAWN, THEM) & ~FILE_H_MASK) >> 7);
    mobility_mask |= this->blocked_pawn_bb[this->side] | pos.piece_bb(KING, US);
    mobility_mask = ~mobility_mask;

    u64 occupancy = pos.occupancy_bb();

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
            u64 atks_bb = lookups::attacks(pt, sq, occupancy);
            value += piece_mobility[pt][popcnt(atks_bb & mobility_mask)];

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

    // King shelter
    u64 pawn_bb = pos.piece_bb(PAWN, US);
    auto shelter = lookups::king_shelter_masks(ksq);
    value += close_shelter * popcnt(pawn_bb & shelter.first);
    value += far_shelter * popcnt(pawn_bb & shelter.second);

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
        if (forward & occupancy)
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
    for (int side = US; side <= THEM; ++side) {
        this->side = side;

        score += eval_pawns();
        score += eval_pieces();

        pos.flip();
        score = -score;
    }

    for (int side = US; side <= THEM; ++side) {
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
