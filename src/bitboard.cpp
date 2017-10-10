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
    u64 pawn(u32 square) { return pawn_attacks[side][square]; }
    u64 knight(u32 square) { return knight_attacks[square]; }
    u64 king(u32 square) { return king_attacks[square]; }

    u64 bishop(u32 square, u64 occupancy)
    {
        u64 s_ne, s_nw, s_se, s_sw;
        u64 ne_mask, nw_mask, se_mask, sw_mask;
        u64 sq_bb = BB(square);

        ne_mask = ~(occupancy | RANK_1_MASK | FILE_H_MASK);
        nw_mask = ~(occupancy | RANK_1_MASK | FILE_A_MASK);
        se_mask = ~(occupancy | RANK_8_MASK | FILE_H_MASK);
        sw_mask = ~(occupancy | RANK_8_MASK | FILE_A_MASK);

        s_ne = (sq_bb << 7) & ne_mask;
        s_nw = (sq_bb << 9) & nw_mask;
        s_se = (sq_bb >> 9) & se_mask;
        s_sw = (sq_bb >> 7) & sw_mask;

        for (u32 times = 0; times < 6; ++times) {
            s_ne |= (s_ne << 7) & ne_mask;
            s_nw |= (s_nw << 9) & nw_mask;
            s_se |= (s_se >> 9) & se_mask;
            s_sw |= (s_sw >> 7) & sw_mask;
        }

        return s_ne | s_nw | s_se | s_sw;
    }

    u64 rook(u32 square, u64 occupancy)
    {
        u64 s_n, s_w, s_s, s_e;
        u64 n_mask, w_mask, s_mask, e_mask;
        u64 sq_bb = BB(square);

        n_mask = ~(occupancy | RANK_1_MASK);
        w_mask = ~(occupancy | FILE_A_MASK);
        s_mask = ~(occupancy | RANK_8_MASK);
        e_mask = ~(occupancy | FILE_H_MASK);

        s_n = (sq_bb << 8) & n_mask;
        s_w = (sq_bb << 1) & w_mask;
        s_s = (sq_bb >> 8) & s_mask;
        s_e = (sq_bb >> 1) & e_mask;

        for (u32 times = 0; times < 6; ++times) {
            s_n |= (s_n << 8) & n_mask;
            s_w |= (s_w << 1) & w_mask;
            s_s |= (s_s >> 8) & s_mask;
            s_e |= (s_e >> 1) & e_mask;
        }

        return s_n | s_w | s_s | s_e;
    }

    u64 queen(u32 square, u64 occupancy)
    {
        u64 s_ne, s_nw, s_se, s_sw;
        u64 ne_mask, nw_mask, se_mask, sw_mask;
        u64 s_n, s_w, s_s, s_e;
        u64 n_mask, w_mask, s_mask, e_mask;
        u64 sq_bb = BB(square);

        ne_mask = ~(occupancy | RANK_1_MASK | FILE_H_MASK);
        nw_mask = ~(occupancy | RANK_1_MASK | FILE_A_MASK);
        se_mask = ~(occupancy | RANK_8_MASK | FILE_H_MASK);
        sw_mask = ~(occupancy | RANK_8_MASK | FILE_A_MASK);
        n_mask = ~(occupancy | RANK_1_MASK);
        w_mask = ~(occupancy | FILE_A_MASK);
        s_mask = ~(occupancy | RANK_8_MASK);
        e_mask = ~(occupancy | FILE_H_MASK);

        s_ne = (sq_bb << 7) & ne_mask;
        s_nw = (sq_bb << 9) & nw_mask;
        s_se = (sq_bb >> 9) & se_mask;
        s_sw = (sq_bb >> 7) & sw_mask;
        s_n = (sq_bb << 8) & n_mask;
        s_w = (sq_bb << 1) & w_mask;
        s_s = (sq_bb >> 8) & s_mask;
        s_e = (sq_bb >> 1) & e_mask;

        for (u32 times = 0; times < 6; ++times) {
            s_ne |= (s_ne << 7) & ne_mask;
            s_nw |= (s_nw << 9) & nw_mask;
            s_se |= (s_se >> 9) & se_mask;
            s_sw |= (s_sw >> 7) & sw_mask;
            s_n |= (s_n << 8) & n_mask;
            s_w |= (s_w << 1) & w_mask;
            s_s |= (s_s >> 8) & s_mask;
            s_e |= (s_e >> 1) & e_mask;
        }

        return s_ne | s_nw | s_se | s_sw | s_n | s_w | s_s | s_e;
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
