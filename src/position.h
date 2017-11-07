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

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include "definitions.h"
#include "lookups.h"

namespace castling
{
    extern bool is_frc;
    extern i32 rook_sqs[2];
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
    u64 get_hash_key() const;
    std::uint8_t get_castling_rights() const;
    std::uint8_t get_half_moves() const;
    bool is_flipped() const;
    bool is_kingside(i32 sq, i32 c) const;
    bool is_queenside(i32 sq, i32 c) const;
    i32 get_ep_sq() const;
    u64 occupancy_bb() const;
    u64 piece_bb(i32 pt) const;
    u64 color_bb(i32 c) const;
    u64 piece_bb(i32 pt, i32 c) const;
    i32 piece_on(i32 sq) const;
    bool check_piece_on(i32 sq, i32 pt) const;
    i32 position_of(i32 pt, i32 c) const;
    u64 attackers_to(i32 sq) const;
    u64 attackers_to(i32 sq, i32 by_side) const;
    u64 in_check(i32 side) const;
    void generate_movelist(std::vector<Move>& mlist) const;
    void generate_quiesce_movelist(std::vector<Move>& mlist) const;

    // Operations
    void flip();
    bool is_repetition() const;
    u64 perft(i32 depth, bool root=true) const;
    int evaluate();
    Move best_move();
    bool make_move(Move move);

private:
    // Internal operations
    void clear();
    void inc_half_moves();
    void reset_half_moves();
    void put_piece(i32 sq, i32 pt, i32 c);
    void remove_piece(i32 sq, i32 pt, i32 c);
    void move_piece(i32 from, i32 to, i32 pt, i32 c);
    u64 calc_hash();

    // Data members
    u64 bb[6];
    u64 color[2];
    bool flipped;
    i32 ep_sq;
    std::uint8_t castling_rights;
    std::uint8_t half_moves;
    u64 hash_key;
    std::vector<u64> prev_hash_keys;
};

inline Position::Position() { this->clear(); }

inline u64 Position::get_hash_key() const { return this->hash_key; }
inline std::uint8_t Position::get_castling_rights() const { return this->castling_rights; }
inline std::uint8_t Position::get_half_moves() const { return this->half_moves; }
inline bool Position::is_flipped() const { return this->flipped; }
inline bool Position::is_kingside(i32 sq, i32 c) const { return sq > this->position_of(KING, c); }
inline bool Position::is_queenside(i32 sq, i32 c) const { return sq < this->position_of(KING, c); }
inline i32 Position::get_ep_sq() const { return this->ep_sq; }
inline u64 Position::occupancy_bb() const { return this->color_bb(US) ^ this->color_bb(THEM); }
inline u64 Position::piece_bb(i32 pt) const { return this->bb[pt]; }
inline u64 Position::color_bb(i32 c) const { return this->color[c]; }
inline u64 Position::piece_bb(i32 pt, i32 c) const { return this->bb[pt] & this->color[c]; }
inline i32 Position::position_of(i32 pt, i32 c) const { return fbitscan(piece_bb(pt, c)); }
inline bool Position::check_piece_on(i32 sq, i32 pt) const { return BB(sq) & this->piece_bb(pt); }

inline void Position::inc_half_moves() { ++this->half_moves; }
inline void Position::reset_half_moves() { this->half_moves = 0; this->prev_hash_keys.clear(); }

inline void Position::put_piece(i32 sq, i32 pt, i32 c)
{
    assert(piece_on(sq) == NO_PIECE);
    u64 bb = BB(sq);
    this->bb[pt] ^= bb;
    this->color[c] ^= bb;
    this->hash_key ^= lookups::psq_key(c, pt, sq);
}

inline void Position::remove_piece(i32 sq, i32 pt, i32 c)
{
    u64 bb = BB(sq);
    assert(this->bb[pt] & this->color[c] & bb);
    this->bb[pt] ^= bb;
    this->color[c] ^= bb;
    this->hash_key ^= lookups::psq_key(c, pt, sq);
}

inline void Position::move_piece(i32 from, i32 to, i32 pt, i32 c)
{
    assert(from != to);
    u64 bb = BB(from) ^ BB(to);
    this->bb[pt] ^= bb;
    this->color[c] ^= bb;
    this->hash_key ^= lookups::psq_key(c, pt, from)
                    ^ lookups::psq_key(c, pt, to);
}

#endif
