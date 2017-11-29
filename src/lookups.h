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

    extern u64 psq_key(i32 c, i32 pt, i32 sq);
    extern u64 castle_key(i32 rights);
    extern u64 ep_key(i32 sq);
    extern u64 stm_key();

    extern i32 distance(i32 from, i32 to);
    extern u64 ray(i32 from, i32 to);
    extern u64 xray(i32 from, i32 to);
    extern u64 full_ray(i32 from, i32 to);
    extern u64 intervening_sqs(i32 from, i32 to);
    extern u64 adjacent_files(i32 square);

    extern u64 passed_pawn_mask(i32 square);
    extern u64 king_danger_zone(i32 square);

    extern u64 north(i32 square);
    extern u64 south(i32 square);
    extern u64 east(i32 square);
    extern u64 west(i32 square);
    extern u64 northeast(i32 square);
    extern u64 northwest(i32 square);
    extern u64 southeast(i32 square);
    extern u64 southwest(i32 square);

    extern u64 pawn(i32 square, i32 side);
    extern u64 knight(i32 square);
    extern u64 bishop(i32 square);
    extern u64 rook(i32 square);
    extern u64 queen(i32 square);
    extern u64 bishop(i32 square, u64 occupancy);
    extern u64 rook(i32 square, u64 occupancy);
    extern u64 queen(i32 square, u64 occupancy);
    extern u64 king(i32 square);
    extern u64 attacks(i32 piece_type, i32 square, u64 occupancy, i32 side=US);
}

#endif
