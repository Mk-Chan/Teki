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

namespace castling
{
    u32 rook_sqs[2][2];
    std::uint8_t spoilers[64];
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
            u32 c = ((BB(sq ^ 56) & this->color[WHITE]) && !this->flipped)
                 || ((BB(sq) & this->color[WHITE]) && this->flipped)
                  ? WHITE
                  : BLACK;
            std::cout << piece_char(piece, c) << " ";
        }
    }
    std::cout << std::endl;
}

void Position::init(std::stringstream& stream)
{
    // Clear everything
    for (u32 i = 0; i < 6; ++i)
        this->bb[i] = 0;
    for (u32 i = 0; i < 2; ++i)
        this->color[i] = 0;
    this->flipped = false;
    this->ep_sq = INVALID_SQ;
    this->castling_rights = 0;
    this->half_moves = 0;
    this->hash_keys.reserve(100);

    // Initialize the castling spoilers
    for (u32 sq = A1; sq < NUM_SQUARES; ++sq)
        castling::spoilers[sq] = 15;

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
            if (pt == KING)
            {
                castling::spoilers[sq] = pc == WHITE ? 12 : 3;
            }
            else if (pt == ROOK)
            {
                switch (sq) {
                case H1:
                    castling::rook_sqs[pc][KINGSIDE] = sq;
                    castling::spoilers[sq] = 14;
                    break;
                case H8:
                    castling::rook_sqs[pc][KINGSIDE] = sq;
                    castling::spoilers[sq] = 11;
                    break;
                case A1:
                    castling::rook_sqs[pc][QUEENSIDE] = sq;
                    castling::spoilers[sq] = 13;
                    break;
                case A8:
                    castling::rook_sqs[pc][QUEENSIDE] = sq;
                    castling::spoilers[sq] = 7;
                    break;
                default:
                    break;
                }
            }
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
        switch (c) {
        case 'K': this->castling_rights |= WHITE_OO; break;
        case 'Q': this->castling_rights |= WHITE_OOO; break;
        case 'k': this->castling_rights |= BLACK_OO; break;
        case 'q': this->castling_rights |= BLACK_OOO; break;
        case '-': break;
        default: break; // FRC or error
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
}
