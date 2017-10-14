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
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

namespace castling
{
    extern bool is_frc;
    extern u32 rook_sqs[2];
    extern u8 spoilers[64];
}

class Position
{
public:
    // Constructors
    Position();
    void init(std::stringstream& stream);

    // Misc
    void display();

    // Getters
    std::uint8_t get_castling_rights() const;
    std::uint8_t get_half_moves() const;
    bool is_flipped() const;
    bool is_kingside(u32 sq, u32 c) const;
    bool is_queenside(u32 sq, u32 c) const;
    u32 get_ep_sq() const;
    u64 get_hash_key() const;
    u64 occupancy_bb() const;
    u64 piece_bb(u32 pt) const;
    u64 color_bb(u32 c) const;
    u64 piece_bb(u32 pt, u32 c) const;
    u32 piece_on(u32 sq) const;
    bool check_piece_on(u32 sq, u32 pt) const;
    u32 position_of(u32 pt, u32 c) const;
    //u64 get_hash();
    u64 attackers_to(u32 sq) const;
    u64 attackers_to(u32 sq, u32 by_side) const;
    u64 in_check(u32 side) const;

    // Operations
    bool make_move(Move move);
    std::vector<u32> get_movelist();
    u64 perft(u32 depth, bool root=true);

private:
    // Internal operations
    void clear();
    void inc_half_moves();
    void reset_half_moves();
    void flip();
    void put_piece(u32 sq, u32 pt, u32 c);
    void remove_piece(u32 sq, u32 pt, u32 c);
    void move_piece(u32 from, u32 to, u32 pt, u32 c);

    // Data members
    u64 bb[6];
    u64 color[2];
    bool flipped;
    u32 ep_sq;
    std::uint8_t castling_rights;
    std::uint8_t half_moves;
    std::vector<u64> hash_keys;
};

inline Position::Position() { this->clear(); }

inline u64 Position::get_hash_key() const { return this->hash_keys.back(); }
inline std::uint8_t Position::get_castling_rights() const { return this->castling_rights; }
inline std::uint8_t Position::get_half_moves() const { return this->half_moves; }
inline bool Position::is_flipped() const { return this->flipped; }
inline bool Position::is_kingside(u32 sq, u32 c) const { return sq > this->position_of(KING, c); }
inline bool Position::is_queenside(u32 sq, u32 c) const { return sq < this->position_of(KING, c); }
inline u32 Position::get_ep_sq() const { return this->ep_sq; }
inline u64 Position::occupancy_bb() const { return this->color_bb(US) ^ this->color_bb(THEM); }
inline u64 Position::piece_bb(u32 pt) const { return this->bb[pt]; }
inline u64 Position::color_bb(u32 c) const { return this->color[c]; }
inline u64 Position::piece_bb(u32 pt, u32 c) const { return this->bb[pt] & this->color[c]; }
inline u32 Position::position_of(u32 pt, u32 c) const { return fbitscan(piece_bb(pt, c)); }
inline bool Position::check_piece_on(u32 sq, u32 pt) const { return BB(sq) & this->piece_bb(pt); }

inline void Position::inc_half_moves() { ++this->half_moves; }
inline void Position::reset_half_moves() { this->half_moves = 0; }

inline void Position::put_piece(u32 sq, u32 pt, u32 c)
{
    assert(piece_on(sq) == NO_PIECE);
    u64 bb = BB(sq);
    this->bb[pt] ^= bb;
    this->color[c] ^= bb;
}

inline void Position::remove_piece(u32 sq, u32 pt, u32 c)
{
    u64 bb = BB(sq);
    assert(this->bb[pt] & this->color[c] & bb);
    this->bb[pt] ^= bb;
    this->color[c] ^= bb;
}

inline void Position::move_piece(u32 from, u32 to, u32 pt, u32 c)
{
    assert(from != to);
    u64 bb = BB(from) ^ BB(to);
    this->bb[pt] ^= bb;
    this->color[c] ^= bb;
}

#endif
