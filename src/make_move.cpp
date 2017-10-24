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

std::string get_move_string(Move move, bool flipped)
{
    u32 from = from_sq(move),
        to = to_sq(move);

    if (flipped) {
        from ^= 56;
        to ^= 56;
    }

    std::string move_string;
    move_string.push_back('a' + file_of(from));
    move_string.push_back('1' + rank_of(from));
    move_string.push_back('a' + file_of(to));
    move_string.push_back('1' + rank_of(to));

    u32 prom = prom_type(move);
    switch (prom) {
    case QUEEN:
        move_string.push_back('q');
        break;
    case KNIGHT:
        move_string.push_back('n');
        break;
    case BISHOP:
        move_string.push_back('b');
        break;
    case ROOK:
        move_string.push_back('r');
        break;
    default:
        break;
    }
    return move_string;
}

bool Position::make_move(Move move)
{
    u32 from = from_sq(move),
        to = to_sq(move);

    this->prev_hash_keys.push_back(this->hash_key);

    if (this->ep_sq != INVALID_SQ)
        this->ep_sq = INVALID_SQ;

    this->castling_rights &= castling::spoilers[from]
                           & castling::spoilers[to];

    if (this->check_piece_on(from, PAWN))
        this->reset_half_moves();
    else
        this->inc_half_moves();

    switch (move & MOVE_TYPE_MASK) {
        case NORMAL:
            this->move_piece(from, to, this->piece_on(from), US);
            break;
        case CAPTURE:
            this->remove_piece(to, this->piece_on(to), THEM);
            this->move_piece(from, to, this->piece_on(from), US);
            this->reset_half_moves();
            break;
        case DOUBLE_PUSH:
            this->move_piece(from, to, PAWN, US);
            this->ep_sq = from + 8;
            break;
        case ENPASSANT:
            this->move_piece(from, to, PAWN, US);
            this->remove_piece(to - 8, PAWN, THEM);
            break;
        case CASTLING:
            u32 rfrom, rto;
            switch (to) {
            case C1:
                rto = D1;
                rfrom = castling::rook_sqs[QUEENSIDE];
                break;
            case G1:
                rto = F1;
                rfrom = castling::rook_sqs[KINGSIDE];
                break;
            default:
                rto = rfrom = -1;
                break;
            }
            this->remove_piece(rfrom, ROOK, US);
            this->remove_piece(from, KING, US);
            this->put_piece(rto, ROOK, US);
            this->put_piece(to, KING, US);
            break;
        case PROM_CAPTURE:
            this->remove_piece(to, this->piece_on(to), THEM);
            this->remove_piece(from, PAWN, US);
            this->put_piece(to, prom_type(move), US);
            break;
        case PROMOTION:
            this->remove_piece(from, PAWN, US);
            this->put_piece(to, prom_type(move), US);
            break;
        default:
            std::cout << "MOVE TYPE ERROR!" << std::endl;
            break;
    }

    if (this->in_check(US))
        return false;

    this->flip();

    this->hash_key = this->calc_hash();

    return true;
}
