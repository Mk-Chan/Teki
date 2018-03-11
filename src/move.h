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

#ifndef MOVE_H
#define MOVE_H

#include <string>
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
    CAPTURE_MASK = CAPTURE | PROM_CAPTURE | ENPASSANT,
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
    CAPTURE_TYPE_MASK = 7 << 22
};

inline u32 from_sq(Move move) { return move & 0x3f; }
inline u32 to_sq(Move move) { return (move & 0xfc0) >> 6; }
inline u32 prom_type(Move move) { return (move & PROMOTION_TYPE_MASK) >> 19; }
inline u32 cap_type(Move move) { return (move & CAPTURE_TYPE_MASK) >> 22; }

inline Move get_move(u32 from, u32 to, u32 move_type, u32 cap_type=CAP_NONE, u32 prom_type=PROM_NONE)
{
    assert(from < 64 && to < 64 && move_type);
    return from | (to << 6) | move_type | prom_type | (cap_type << 22);
}

extern std::string get_move_string(Move move, bool flipped);

#endif
