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

#include <iostream>
#include "definitions.h"
#include "bitboard.h"

u64 pawn_attacks[2][64];
u64 knight_attacks[64];
u64 king_attacks[64];

void init_non_sliders()
{
    for (u32 sq = A1; sq < NUM_SQUARES; ++sq) {
        king_attacks[sq] = knight_attacks[sq] = 0;
        pawn_attacks[US][sq] = pawn_attacks[THEM][sq] = 0;

        // Pawn attacks
        pawn_attacks[US][sq] |= ((BB(sq + 7) & ~FILE_H_MASK)
                               | (BB(sq + 9) & ~FILE_A_MASK))
                               & ~RANK_1_MASK;
        pawn_attacks[THEM][sq] |= ((BB(sq - 7) & ~FILE_A_MASK)
                                 | (BB(sq - 9) & ~FILE_H_MASK))
                                 & ~RANK_8_MASK;

        // Knight attacks
        knight_attacks[sq] |= BB(sq + 17) & ~(FILE_A_MASK | RANK_2_MASK | RANK_1_MASK);
        knight_attacks[sq] |= BB(sq + 15) & ~(FILE_H_MASK | RANK_2_MASK | RANK_1_MASK);
        knight_attacks[sq] |= BB(sq - 17) & ~(FILE_H_MASK | RANK_7_MASK | RANK_8_MASK);
        knight_attacks[sq] |= BB(sq - 15) & ~(FILE_A_MASK | RANK_7_MASK | RANK_8_MASK);
        knight_attacks[sq] |= BB(sq - 10) & ~(FILE_H_MASK | FILE_G_MASK | RANK_8_MASK);
        knight_attacks[sq] |= BB(sq + 6)  & ~(FILE_H_MASK | FILE_G_MASK | RANK_1_MASK);
        knight_attacks[sq] |= BB(sq + 10) & ~(FILE_A_MASK | FILE_B_MASK | RANK_1_MASK);
        knight_attacks[sq] |= BB(sq - 6)  & ~(FILE_A_MASK | FILE_B_MASK | RANK_8_MASK);

        // King attacks
        king_attacks[sq] |= BB(sq + 8) & ~RANK_1_MASK;
        king_attacks[sq] |= BB(sq - 8) & ~RANK_8_MASK;
        king_attacks[sq] |= BB(sq + 1) & ~FILE_A_MASK;
        king_attacks[sq] |= BB(sq - 1) & ~FILE_H_MASK;
        king_attacks[sq] |= BB(sq + 9) & ~(FILE_A_MASK | RANK_1_MASK);
        king_attacks[sq] |= BB(sq - 9) & ~(FILE_H_MASK | RANK_8_MASK);
        king_attacks[sq] |= BB(sq + 7) & ~(FILE_H_MASK | RANK_1_MASK);
        king_attacks[sq] |= BB(sq - 7) & ~(FILE_A_MASK | RANK_8_MASK);
    }
}

namespace lookups
{
    void init()
    {
        init_non_sliders();
        //init_sliders();
    }

    template<u32 side>
    inline u64 pawn(u32 square) { return pawn_attacks[side][square]; }
    inline u64 knight(u32 square) { return knight_attacks[square]; }
    inline u64 king(u32 square) { return king_attacks[square]; }

    inline u64 bishop(u32 square, u64 occupancy)
    {
        return 0;
    }
    inline u64 rook(u32 square, u64 occupancy)
    {
        return 0;
    }
    inline u64 queen(u32 square, u64 occupancy)
    {
        return 0;
    }
}

void print_bb(u64 bb)
{
    for (u32 sq = 0; sq < NUM_SQUARES; ++sq) {
        if (sq && !(sq & 7))
            std::cout << '\n';

        if (BB(sq ^ 56) & bb)
            std::cout << "X  ";
        else
            std::cout << "-  ";
    }
    std::cout << std::endl;
}
