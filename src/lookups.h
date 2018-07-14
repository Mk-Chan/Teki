/*
 * Teki-MCTS
 * Copyright (C) 2018  Manik Charan
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LOOKUPS_H
#define LOOKUPS_H

#include "definitions.h"

#define RANK_1_MASK (u64(0xff))
#define RANK_2_MASK (u64(0xff00))
#define RANK_3_MASK (u64(0xff0000))
#define RANK_4_MASK (u64(0xff000000))
#define RANK_5_MASK (u64(0xff00000000))
#define RANK_6_MASK (u64(0xff0000000000))
#define RANK_7_MASK (u64(0xff000000000000))
#define RANK_8_MASK (u64(0xff00000000000000))

#define FILE_A_MASK (u64(0x0101010101010101))
#define FILE_B_MASK (u64(0x0202020202020202))
#define FILE_C_MASK (u64(0x0404040404040404))
#define FILE_D_MASK (u64(0x0808080808080808))
#define FILE_E_MASK (u64(0x1010101010101010))
#define FILE_F_MASK (u64(0x2020202020202020))
#define FILE_G_MASK (u64(0x4040404040404040))
#define FILE_H_MASK (u64(0x8080808080808080))

namespace lookups
{
    extern void init();

    extern u64 psq_key(int c, int pt, int sq);
    extern u64 castle_key(int rights);
    extern u64 ep_key(int sq);
    extern u64 stm_key();

    extern int distance(int from, int to);
    extern u64 ray(int from, int to);
    extern u64 xray(int from, int to);
    extern u64 full_ray(int from, int to);
    extern u64 intervening_sqs(int from, int to);
    extern u64 adjacent_files(int square);
    extern u64 adjacent_sqs(int square);
    extern u64 file_mask(int square);
    extern u64 rank_mask(int square);

    extern u64 north(int square);
    extern u64 south(int square);
    extern u64 east(int square);
    extern u64 west(int square);
    extern u64 northeast(int square);
    extern u64 northwest(int square);
    extern u64 southeast(int square);
    extern u64 southwest(int square);
    extern u64 north_region(int square);
    extern u64 south_region(int square);
    extern u64 east_region(int square);
    extern u64 west_region(int square);

    extern u64 pawn(int square, int side);
    extern u64 knight(int square);
    extern u64 bishop(int square);
    extern u64 rook(int square);
    extern u64 queen(int square);
    extern u64 bishop(int square, u64 occupancy);
    extern u64 rook(int square, u64 occupancy);
    extern u64 queen(int square, u64 occupancy);
    extern u64 king(int square);
    extern u64 attacks(int piece_type, int square, u64 occupancy, int side=US);

    extern u64 passed_pawn_mask(int square);
    extern u64 king_danger_zone(int square);
    extern std::pair<u64, u64> king_shelter_masks(int square);
}

#endif
