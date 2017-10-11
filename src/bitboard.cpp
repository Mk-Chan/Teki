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

u32 distance_val[64][64];
u64 ahead_bb[64];
u64 behind_bb[64];
u64 ray_bb[64][64];
u64 xray_bb[64][64];
u64 full_ray_bb[64][64];
u64 intervening_ray_bb[64][64];
u64 pawn_attacks[2][64];
u64 knight_attacks[64];
u64 bishop_attacks[64];
u64 rook_attacks[64];
u64 queen_attacks[64];
u64 king_attacks[64];

u64 north_bb[64];
u64 south_bb[64];
u64 east_bb[64];
u64 west_bb[64];
u64 northeast_bb[64];
u64 northwest_bb[64];
u64 southeast_bb[64];
u64 southwest_bb[64];

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

void init_pseudo_sliders()
{
    for (u32 sq = A1; sq < NUM_SQUARES; ++sq) {
        bishop_attacks[sq] = lookups::northeast(sq) | lookups::northwest(sq)
                           | lookups::southeast(sq) | lookups::southwest(sq);
        rook_attacks[sq] = lookups::north(sq) | lookups::south(sq)
                         | lookups::east(sq) | lookups::west(sq);
        queen_attacks[sq] = rook_attacks[sq] | bishop_attacks[sq];
    }
}

void init_misc()
{
	int i, j, high, low;
	for (i = 0; i < 64; i++) {
        ahead_bb[i] = behind_bb[i] = 0;
        for (j = 0; j < i; ++j)
            behind_bb[i] |= BB(j);
        for (j = i + 1; j < 64; ++j)
            ahead_bb[i] |= BB(j);
		for (j = 0; j < 64; j++) {
			distance_val[i][j] = std::max(abs(i32(rank_of(i)) - i32(rank_of(j))),
                                      abs(i32(file_of(i)) - i32(file_of(j))));
			intervening_ray_bb[i][j] = 0;
			if (i == j)
				continue;
			high = j;
			if (i > j) {
				high = i;
				low = j;
			}
			else
				low = i;
			if (file_of(high) == file_of(low))
            {
                full_ray_bb[i][j] =
                    (lookups::rook(high) & lookups::rook(low)) | BB(i) | BB(j);
                xray_bb[low][high] = lookups::north(low);
                xray_bb[high][low] = lookups::south(high);
                ray_bb[i][j] = BB(high);
				for (high -= 8; high >= low; high -= 8) {
                    ray_bb[i][j] |= BB(high);
                    if (high != low)
                        intervening_ray_bb[i][j] |= BB(high);
                }
			}
			else if (rank_of(high) == rank_of(low))
            {
                full_ray_bb[i][j] =
                    (lookups::rook(high) & lookups::rook(low)) | BB(i) | BB(j);
                xray_bb[low][high] = lookups::east(low);
                xray_bb[high][low] = lookups::west(high);
                ray_bb[i][j] = BB(high);
				for (--high; high >= low; high--) {
                    ray_bb[i][j] |= BB(high);
                    if (high != low)
                        intervening_ray_bb[i][j] |= BB(high);
                }
			}
			else if (rank_of(high) - rank_of(low) == file_of(high) - file_of(low))
            {
                full_ray_bb[i][j] =
                    (lookups::bishop(high) & lookups::bishop(low)) | BB(i) | BB(j);
                xray_bb[low][high] = lookups::northeast(low);
                xray_bb[high][low] = lookups::southwest(high);
                ray_bb[i][j] = BB(high);
				for (high -= 9; high >= low; high -= 9) {
                    ray_bb[i][j] |= BB(high);
                    if (high != low)
                        intervening_ray_bb[i][j] |= BB(high);
                }
			}
			else if (rank_of(high) - rank_of(low) == file_of(low) - file_of(high))
            {
                full_ray_bb[i][j] =
                    (lookups::bishop(high) & lookups::bishop(low)) | BB(i) | BB(j);
                xray_bb[low][high] = lookups::northwest(low);
                xray_bb[high][low] = lookups::southeast(high);
                ray_bb[i][j] = BB(high);
				for (high -= 7; high >= low; high -= 7) {
                    ray_bb[i][j] |= BB(high);
                    if (high != low)
                        intervening_ray_bb[i][j] |= BB(high);
                }
			}
		}
	}
}

void init_directions()
{
    u64 ne, nw, se, sw;
    u64 n, w, s, e;
    u64 ne_mask, nw_mask, se_mask, sw_mask;
    u64 n_mask, w_mask, s_mask, e_mask;
    nw_mask = ~(RANK_1_MASK | FILE_H_MASK);
    ne_mask = ~(RANK_1_MASK | FILE_A_MASK);
    sw_mask = ~(RANK_8_MASK | FILE_H_MASK);
    se_mask = ~(RANK_8_MASK | FILE_A_MASK);
    n_mask = ~(RANK_1_MASK);
    e_mask = ~(FILE_A_MASK);
    s_mask = ~(RANK_8_MASK);
    w_mask = ~(FILE_H_MASK);

    for (u32 sq = A1; sq < NUM_SQUARES; ++sq) {
        u64 sq_bb = BB(sq);

        nw = (sq_bb << 7) & nw_mask;
        ne = (sq_bb << 9) & ne_mask;
        sw = (sq_bb >> 9) & sw_mask;
        se = (sq_bb >> 7) & se_mask;
        n = (sq_bb << 8) & n_mask;
        e = (sq_bb << 1) & e_mask;
        s = (sq_bb >> 8) & s_mask;
        w = (sq_bb >> 1) & w_mask;

        for (u32 times = 0; times < 6; ++times) {
            nw |= (nw << 7) & nw_mask;
            ne |= (ne << 9) & ne_mask;
            sw |= (sw >> 9) & sw_mask;
            se |= (se >> 7) & se_mask;
            n |= (n << 8) & n_mask;
            e |= (e << 1) & e_mask;
            s |= (s >> 8) & s_mask;
            w |= (w >> 1) & w_mask;
        }

        north_bb[sq] = n;
        south_bb[sq] = s;
        east_bb[sq] = e;
        west_bb[sq] = w;
        northeast_bb[sq] = ne;
        northwest_bb[sq] = nw;
        southeast_bb[sq] = se;
        southwest_bb[sq] = sw;
    }
}

namespace lookups
{
    void init()
    {
        init_non_sliders();
        init_directions();
        init_pseudo_sliders();
        init_misc();
    }

    u32 distance(u32 from, u32 to) { return distance_val[from][to]; }
    u64 ray(u32 from, u32 to) { return ray_bb[from][to]; }
    u64 xray(u32 from, u32 to) { return xray_bb[from][to]; }
    u64 full_ray(u32 from, u32 to) { return full_ray_bb[from][to]; }
    u64 intervening_sqs(u32 from, u32 to) { return intervening_ray_bb[from][to]; }
    u64 ahead(u32 square) { return ahead_bb[square]; }
    u64 behind(u32 square) { return behind_bb[square]; }
    u64 north(u32 square) { return north_bb[square]; }
    u64 south(u32 square) { return south_bb[square]; }
    u64 east(u32 square) { return east_bb[square]; }
    u64 west(u32 square) { return west_bb[square]; }
    u64 northeast(u32 square) { return northeast_bb[square]; }
    u64 northwest(u32 square) { return northwest_bb[square]; }
    u64 southeast(u32 square) { return southeast_bb[square]; }
    u64 southwest(u32 square) { return southwest_bb[square]; }
    u64 pawn(u32 square, u32 side) { return pawn_attacks[side][square]; }
    u64 knight(u32 square) { return knight_attacks[square]; }
    u64 bishop(u32 square) { return bishop_attacks[square]; }
    u64 rook(u32 square) { return rook_attacks[square]; }
    u64 queen(u32 square) { return queen_attacks[square]; }
    u64 king(u32 square) { return king_attacks[square]; }

    u64 bishop(u32 square, u64 occupancy)
    {
        u64 atk = bishop(square);
        u64 nw_blockers = (northwest(square) & occupancy) | BB(A8);
        u64 ne_blockers = (northeast(square) & occupancy) | BB(H8);
        u64 sw_blockers = (southwest(square) & occupancy) | BB(A1);
        u64 se_blockers = (southeast(square) & occupancy) | BB(H1);

        atk &= ~northwest(fbitscan(nw_blockers));
        atk &= ~northeast(fbitscan(ne_blockers));
        atk &= ~southwest(rbitscan(sw_blockers));
        atk &= ~southeast(rbitscan(se_blockers));

        return atk;

    }

    u64 rook(u32 square, u64 occupancy)
    {
        u64 atk = rook(square);
        u64 n_blockers = (north(square) & occupancy) | BB(H8);
        u64 s_blockers = (south(square) & occupancy) | BB(A1);
        u64 w_blockers = (west(square) & occupancy) | BB(A1);
        u64 e_blockers = (east(square) & occupancy) | BB(H8);

        atk &= ~north(fbitscan(n_blockers));
        atk &= ~south(rbitscan(s_blockers));
        atk &= ~west(rbitscan(w_blockers));
        atk &= ~east(fbitscan(e_blockers));

        return atk;
    }

    u64 queen(u32 square, u64 occupancy)
    {
        u64 atk = queen(square);
        u64 nw_blockers = (northwest(square) & occupancy) | BB(A8);
        u64 ne_blockers = (northeast(square) & occupancy) | BB(H8);
        u64 sw_blockers = (southwest(square) & occupancy) | BB(A1);
        u64 se_blockers = (southeast(square) & occupancy) | BB(H1);
        u64 n_blockers = (north(square) & occupancy) | BB(H8);
        u64 s_blockers = (south(square) & occupancy) | BB(A1);
        u64 w_blockers = (west(square) & occupancy) | BB(A1);
        u64 e_blockers = (east(square) & occupancy) | BB(H8);

        atk &= ~northwest(fbitscan(nw_blockers));
        atk &= ~northeast(fbitscan(ne_blockers));
        atk &= ~southwest(rbitscan(sw_blockers));
        atk &= ~southeast(rbitscan(se_blockers));
        atk &= ~north(fbitscan(n_blockers));
        atk &= ~south(rbitscan(s_blockers));
        atk &= ~west(rbitscan(w_blockers));
        atk &= ~east(fbitscan(e_blockers));

        return atk;
    }
}
