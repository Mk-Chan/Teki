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

#ifndef MOVE_H
#define MOVE_H

#include "definitions.h"

enum MoveType
{
    NORMAL = 1 << 12,
    CASTLING = 1 << 13,
    ENPASSANT = 1 << 14,
    PROMOTION = 1 << 15,
    DOUBLE_PUSH = 1 << 16,
    CAPTURE = 1 << 17,
    PROM_CAPTURE = 1 << 18,
    MOVE_TYPE_MASK = NORMAL | CASTLING | ENPASSANT | PROMOTION
                   | DOUBLE_PUSH | CAPTURE | PROM_CAPTURE
};

enum PromotionType
{
    PROM_NONE = 0,
    PROM_TO_KNIGHT = KNIGHT << 19,
    PROM_TO_BISHOP = BISHOP << 19,
    PROM_TO_ROOK = ROOK << 19,
    PROM_TO_QUEEN = QUEEN << 19,
    PROMOTION_TYPE_MASK = 7 << 19
};

enum CaptureType
{
    CAP_NONE = 0,
    CAP_KNIGHT = KNIGHT << 22,
    CAP_BISHOP = BISHOP << 22,
    CAP_ROOK = ROOK << 22,
    CAP_QUEEN = QUEEN << 22,
    CAPTURE_TYPE_MASK = 7 << 22
};

inline u32 from_sq(Move& move) { return move & 0x3f; }
inline u32 to_sq(Move& move) { return (move & 0xfc0) >> 6; }
inline u32 prom_type(Move& move) { return (move & PROMOTION_TYPE_MASK) >> 19; }
inline u32 cap_type(Move& move) { return (move & CAPTURE_TYPE_MASK) >> 22; }

template <MoveType move_type,
          PromotionType prom_type=PROM_NONE,
          CaptureType cap_type=CAP_NONE>
inline Move get_move(u32 from, u32 to)
{
    return from | (to << 6) | move_type | prom_type | cap_type;
}

#endif
