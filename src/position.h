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

#ifndef POSITION_H
#define POSITION_H

#include <utility>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include "definitions.h"
#include "lookups.h"

namespace castling
{
    inline bool is_frc = false;
    inline int rook_sqs[2];
    inline u8 spoilers[64];
}

inline bool allow_ponder = true;

class Position
{
public:
    // Constructors
    Position();
    void init(std::stringstream& stream);

    // Misc
    void display();
    u64 perft(int depth, bool root=true) const;

    // Getters
    u64 get_hash_key() const;
    std::uint8_t get_castling_rights() const;
    std::uint8_t get_half_moves() const;
    bool is_flipped() const;
    bool is_kingside(int sq, int c) const;
    bool is_queenside(int sq, int c) const;
    bool is_passed_pawn(int sq) const;
    int get_ep_sq() const;
    u64 occupancy_bb() const;
    u64 piece_bb(int pt) const;
    u64 color_bb(int c) const;
    u64 piece_bb(int pt, int c) const;
    int piece_on(int sq) const;
    bool check_piece_on(int sq, int pt) const;
    int position_of(int pt, int c) const;
    u64 attackers_to(int sq) const;
    u64 attackers_to(int sq, int by_side) const;
    u64 attackers_to(int sq, u64 occupancy) const;
    u64 attackers_to(int sq, int by_side, u64 occupancy) const;
    u64 checkers_to(int side) const;
    u64 pinned(int to_side) const;
    void generate_in_check_movelist(std::vector<Move>& mlist) const;
    void generate_movelist(std::vector<Move>& mlist) const;
    void generate_quiesce_movelist(std::vector<Move>& mlist) const;
    bool is_repetition() const;
    bool legal_move(Move move) const;
    Move smallest_capture_move(int sq) const;
    int see(int sq) const;

    // Operations
    void flip();
    int evaluate();
    std::pair<Move, Move> best_move();
    void make_move(Move move);
    void make_null_move();

private:
    // Internal operations
    void clear();
    void inc_half_moves();
    void reset_half_moves();
    void clear_prev_hash_keys();
    void put_piece(int sq, int pt, int c);
    void remove_piece(int sq, int pt, int c);
    void move_piece(int from, int to, int pt, int c);
    u64 calc_hash();

    // Data members
    u64 bb[6];
    u64 color[2];
    bool flipped;
    int ep_sq;
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
inline bool Position::is_kingside(int sq, int c) const { return sq > this->position_of(KING, c); }
inline bool Position::is_queenside(int sq, int c) const { return sq < this->position_of(KING, c); }
inline int Position::get_ep_sq() const { return this->ep_sq; }
inline u64 Position::occupancy_bb() const { return this->color_bb(US) ^ this->color_bb(THEM); }
inline u64 Position::piece_bb(int pt) const { return this->bb[pt]; }
inline u64 Position::color_bb(int c) const { return this->color[c]; }
inline u64 Position::piece_bb(int pt, int c) const { return this->bb[pt] & this->color[c]; }
inline int Position::position_of(int pt, int c) const { return fbitscan(piece_bb(pt, c)); }
inline bool Position::check_piece_on(int sq, int pt) const { return BB(sq) & this->piece_bb(pt); }

inline void Position::inc_half_moves() { ++this->half_moves; }
inline void Position::reset_half_moves() { this->half_moves = 0; }
inline void Position::clear_prev_hash_keys() { this->prev_hash_keys.clear(); }

inline void Position::put_piece(int sq, int pt, int c)
{
    assert(piece_on(sq) == NO_PIECE);
    u64 bb = BB(sq);
    this->bb[pt] ^= bb;
    this->color[c] ^= bb;
}

inline void Position::remove_piece(int sq, int pt, int c)
{
    u64 bb = BB(sq);
    assert(this->bb[pt] & this->color[c] & bb);
    this->bb[pt] ^= bb;
    this->color[c] ^= bb;
}

inline void Position::move_piece(int from, int to, int pt, int c)
{
    assert(from != to);
    u64 bb = BB(from) ^ BB(to);
    this->bb[pt] ^= bb;
    this->color[c] ^= bb;
}

#endif
