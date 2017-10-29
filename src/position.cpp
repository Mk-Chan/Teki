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

#include "position.h"
#include "move.h"
#include "lookups.h"

namespace castling
{
    bool is_frc = false;
    u32 rook_sqs[2];
    u8 spoilers[64];
}

char piece_char(u32 pt, u32 c)
{
    char pchar;
    switch (pt) {
    case PAWN: pchar = 'p'; break;
    case KNIGHT: pchar = 'n'; break;
    case BISHOP: pchar = 'b'; break;
    case ROOK: pchar = 'r'; break;
    case QUEEN: pchar = 'q'; break;
    case KING: pchar = 'k'; break;
    default: pchar = 'd'; break;
    }
    return c == WHITE ? std::toupper(pchar) : pchar;
}

void Position::flip()
{
    for (u64* bb = this->bb; bb < this->bb + NUM_PIECE_TYPES; ++bb)
        *bb = __builtin_bswap64(*bb);
    for (u64* bb = this->color; bb < this->color + NUM_COLORS; ++bb)
        *bb = __builtin_bswap64(*bb);

    u64 tmp_color = this->color[1];
    this->color[1] = this->color[0];
    this->color[0] = tmp_color;

    if (this->ep_sq != INVALID_SQ)
        this->ep_sq ^= 56;

    std::uint8_t tmp_cr = (this->castling_rights & 3) << 2;
    this->castling_rights >>= 2;
    this->castling_rights ^= tmp_cr;

    this->flipped = !this->flipped;
}

u32 Position::piece_on(u32 sq) const
{
    u64 sq_bb = BB(sq);
    for (u32 pt = PAWN; pt < NUM_PIECE_TYPES; ++pt)
        if (this->bb[pt] & sq_bb)
            return pt;
    return NO_PIECE;
}

void Position::display()
{
    for (u32 sq = A1; sq < NUM_SQUARES; ++sq) {
        if (sq && !(sq & 7))
            std::cout << '\n';
        u32 piece = this->piece_on(sq ^ 56);
        if (piece == NO_PIECE)
        {
            std::cout << "- ";
        }
        else
        {
            u32 color = ((this->color_bb(US) & BB(sq ^ 56)) && !this->is_flipped())
                     || ((this->color_bb(THEM) & BB(sq ^ 56)) && this->is_flipped())
                      ? WHITE
                      : BLACK;
            std::cout << piece_char(piece, color) << " ";
        }
    }
    std::cout << "\n"
              << "Zobrist key: " << this->hash_key
              << std::endl;
}

void Position::clear()
{
    for (u32 i = 0; i < 6; ++i)
        this->bb[i] = 0;
    for (u32 i = 0; i < 2; ++i)
        this->color[i] = 0;
    this->flipped = false;
    this->ep_sq = INVALID_SQ;
    this->castling_rights = 0;
    this->half_moves = 0;
    this->hash_key = 0;
    this->prev_hash_keys.clear();
    this->prev_hash_keys.reserve(100);
}

void Position::init(std::stringstream& stream)
{
    this->clear();
    for (u32 i = 0 ; i < 64; ++i)
        castling::spoilers[i] = 15;

    if (!castling::is_frc)
    {
        castling::rook_sqs[KINGSIDE] = H1;
        castling::rook_sqs[QUEENSIDE] = A1;
        castling::spoilers[H1] = 14;
        castling::spoilers[H8] = 11;
        castling::spoilers[A1] = 13;
        castling::spoilers[A8] = 7;
        castling::spoilers[E1] = 12;
        castling::spoilers[E8] = 3;
    }

    std::string part;

    // Piece list
    stream >> part;
    u32 index = 0;
    for (u32 i = 0; i < 64;) {
        char c = part[index];
        ++index;
        if (c > '0' && c < '9')
        {
            i += (c - '0');
        }
        else if (c == '/')
        {
            continue;
        }
        else
        {
            u32 sq = i ^ 56;
            u32 pt, pc;
            switch (c) {
            case 'p': pt = PAWN, pc = BLACK; break;
            case 'r': pt = ROOK, pc = BLACK; break;
            case 'n': pt = KNIGHT, pc = BLACK; break;
            case 'b': pt = BISHOP, pc = BLACK; break;
            case 'q': pt = QUEEN, pc = BLACK; break;
            case 'k': pt = KING, pc = BLACK; break;
            case 'P': pt = PAWN, pc = WHITE; break;
            case 'R': pt = ROOK, pc = WHITE; break;
            case 'N': pt = KNIGHT, pc = WHITE; break;
            case 'B': pt = BISHOP, pc = WHITE; break;
            case 'Q': pt = QUEEN, pc = WHITE; break;
            case 'K': pt = KING, pc = WHITE; break;
            default : pt = -1, pc = -1; break; // Error
            }
            this->put_piece(sq, pt, pc);
            ++i;
        }
    }

    // Side to move
    stream >> part;
    bool need_to_flip = part == "b";

    // Castling
    stream >> part;
    for (u32 i = 0; i < part.length(); ++i) {
        char c = part[i];
        if (c == '-')
        {
            break;
        }
        else if (!castling::is_frc)
        {
            switch (c) {
            case 'K': this->castling_rights |= WHITE_OO; break;
            case 'Q': this->castling_rights |= WHITE_OOO; break;
            case 'k': this->castling_rights |= BLACK_OO; break;
            case 'q': this->castling_rights |= BLACK_OOO; break;
            default: break;
            }
        }
        else // TODO: Fischer Random Chess
        {
        }
    }

    // Enpassant square
    stream >> part;
    if (part == "-")
    {
        this->ep_sq = INVALID_SQ;
    }
    else
    {
        this->ep_sq = get_sq(part[0] - 'a', part[1] - '1');
    }

    // Halfmove number
    stream >> this->half_moves;

    // Fullmove number
    u32 full_moves; // dummy
    stream >> full_moves;

    if (need_to_flip)
        this->flip();

    this->hash_key = this->calc_hash();
}

u64 Position::attackers_to(u32 sq) const
{
    u64 occupancy = this->occupancy_bb();
    return (lookups::rook(sq, occupancy) & (piece_bb(ROOK) | piece_bb(QUEEN)))
         | (lookups::bishop(sq, occupancy) & (piece_bb(BISHOP) | piece_bb(QUEEN)))
         | (lookups::knight(sq) & piece_bb(KNIGHT))
         | (lookups::pawn(sq, US) & piece_bb(PAWN, THEM))
         | (lookups::pawn(sq, THEM) & piece_bb(PAWN, US))
         | (lookups::king(sq) & piece_bb(KING));
}

u64 Position::attackers_to(u32 sq, u32 by_side) const
{
    return this->attackers_to(sq) & this->color_bb(by_side);
}

u64 Position::in_check(u32 side) const
{
        return attackers_to(this->position_of(KING, side), !side);
}

u64 Position::calc_hash()
{
    bool flipped = this->is_flipped();
    if (flipped)
        this->flip();

    u64 hash_key = u64(0);
    for (u32 c = WHITE; c <= BLACK; ++c) {
        for (u32 pt = PAWN; pt <= KING; ++pt) {
            u64 bb = this->piece_bb(pt, c);
            while (bb) {
                hash_key ^= lookups::psq_key(c, pt, fbitscan(bb));
                bb &= bb - 1;
            }
        }
    }

    if (this->ep_sq != INVALID_SQ)
        hash_key ^= lookups::ep_key(this->ep_sq);

    hash_key ^= lookups::castle_key(this->castling_rights);

    if (flipped) {
        hash_key ^= lookups::stm_key();
        this->flip();
    }

    return hash_key;
}

u64 Position::perft(u32 depth, bool root) const
{
    if (depth == 0)
        return u64(1);

    std::vector<Move> mlist = this->get_movelist();
    u64 leaves = u64(0);
    for (Move move : mlist) {
        Position pos = *this;
        if (!pos.make_move(move))
            continue;
        u64 count = pos.perft(depth - 1, false);
        leaves += count;
        if (root)
            std::cout << get_move_string(move, this->is_flipped()) << ": "
                      << count << std::endl;
    }

    return leaves;
}
