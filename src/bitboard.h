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

#ifndef BITBOARD_H
#define BITBOARD_H

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
    extern u32 distance(u32 from, u32 to);
    extern u64 ray(u32 from, u32 to);
    extern u64 xray(u32 from, u32 to);
    extern u64 full_ray(u32 from, u32 to);
    extern u64 intervening_sqs(u32 from, u32 to);
    extern u64 ahead(u32 square);
    extern u64 behind(u32 square);
    extern u64 pawn(u32 square, u32 side);
    extern u64 knight(u32 square);
    extern u64 bishop(u32 square);
    extern u64 rook(u32 square);
    extern u64 queen(u32 square);
    extern u64 bishop(u32 square, u64 occupancy);
    extern u64 rook(u32 square, u64 occupancy);
    extern u64 queen(u32 square, u64 occupancy);
    extern u64 king(u32 square);

    extern u64 north(u32 square);
    extern u64 south(u32 square);
    extern u64 east(u32 square);
    extern u64 west(u32 square);
    extern u64 northeast(u32 square);
    extern u64 northwest(u32 square);
    extern u64 southeast(u32 square);
    extern u64 southwest(u32 square);
}

#endif
