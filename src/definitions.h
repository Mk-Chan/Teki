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

#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <cassert>
#include <cinttypes>

typedef std::uint64_t u64;
typedef std::uint32_t u32;
typedef std::uint32_t Move;
typedef std::int32_t i32;

enum Color
{
    WHITE, BLACK,
    US = 0, THEM,
    NUM_COLORS
};

enum Square
{
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
    NUM_SQUARES, INVALID_SQ
};

enum File
{
    FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H,
    NUM_FILES
};

enum Rank
{
    RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8,
    NUM_RANKS
};

enum PieceType
{
    PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING,
    NUM_PIECE_TYPES, NO_PIECE
};

enum CastlingType
{
    KINGSIDE, QUEENSIDE,
    NUM_CASTLING_TYPES
};

enum CastlingRights
{
    WHITE_OO, WHITE_OOO,
    BLACK_OO, BLACK_OOO
};

inline u32 get_sq(u32 rank, u32 file) { return (rank << 3) ^ file; }
inline u32 rank_of(u32 sq) { return sq >> 3; }
inline u32 file_of(u32 sq) { return sq & 7; }

inline u32 popcnt(u64 bb) { return __builtin_popcountll(bb); }
inline u32 fbitscan(u64 bb) { return __builtin_ctzll(bb); }
inline u32 rbitscan(u64 bb) { return __builtin_clzll(bb); }

inline u64 BB(u32 shift) { return u64(1) << shift; }
extern void print_bb(u64 bb);

#endif
