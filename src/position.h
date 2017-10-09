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

#ifndef POSITION_H
#define POSITION_H

#include "definitions.h"
#include <vector>

class Position
{
public:
    // Getters
    std::uint8_t get_castling_rights();
    std::uint8_t get_half_moves();
    bool is_flipped();
    u32 get_ep_sq();
    u64 get_hash_key();
    u64 occupancy_bb();
    u64 piece_bb(u32 pt);
    u64 color_bb(u32 c);
    u64 piece_bb(u32 pt, u32 c);

    // Operations
    void spoil_castling_rights(std::uint8_t spoiler);
    void inc_half_moves();
    void reset_half_moves();
    u32 piece_on(u32 sq);
    void put_piece(u32 pt, u32 sq, u32 c);
    void remove_piece(u32 pt, u32 sq, u32 c);
    void move_piece(u32 pt, u32 from, u32 to, u32 c);

private:
    u64 bb[6];
    u64 color[2];
    bool flipped;
    u32 ep_sq;
    std::uint8_t castling_rights;
    std::uint8_t half_moves;
    u64 hash_key;
    std::vector<u64> prev_hashes;
};

inline u64 Position::get_hash_key() { return this->hash_key; }
inline std::uint8_t Position::get_castling_rights() { return this->castling_rights; }
inline std::uint8_t Position::get_half_moves() { return this->half_moves; }
inline bool Position::is_flipped() { return this->flipped; }
inline u32 Position::get_ep_sq() { return this->ep_sq; }
inline u64 Position::occupancy_bb() { return this->color[WHITE] ^ this->color[BLACK]; }
inline u64 Position::piece_bb(u32 pt) { return this->bb[pt]; }
inline u64 Position::color_bb(u32 pt) { return this->color[pt]; }
inline u64 Position::piece_bb(u32 pt, u32 c) { return this->bb[pt] & this->color[c]; }

inline void Position::spoil_castling_rights(std::uint8_t spoiler) { this->castling_rights &= spoiler; }
inline void Position::inc_half_moves() { ++this->half_moves; }
inline void Position::reset_half_moves() { this->half_moves = 0; }

inline u32 Position::piece_on(u32 sq)
{
    u64 sq_bb = BB(sq);
    for (u32 pt = PAWN; pt < NUM_PIECE_TYPES; ++pt)
        if (this->bb[pt] & sq_bb)
            return pt;
    return NO_PIECE;
}

inline void Position::put_piece(u32 pt, u32 sq, u32 c)
{
    assert(!piece_on(sq));
    u64 bb = BB(sq);
    this->bb[pt] ^= bb;
    this->color[c] ^= bb;
}

inline void Position::remove_piece(u32 pt, u32 sq, u32 c)
{
    u64 bb = BB(sq);
    assert(this->bb[pt] & this->color[c] & bb);
    this->bb[pt] ^= bb;
    this->color[c] ^= bb;
}

inline void Position::move_piece(u32 pt, u32 from, u32 to, u32 c)
{
    assert(from != to);
    u64 bb = BB(from) ^ BB(to);
    this->bb[pt] ^= bb;
    this->color[c] ^= bb;
}

#endif
