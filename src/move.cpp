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

#include "position.h"
#include "move.h"

std::string get_move_string(Move move, bool flipped)
{
    int from = from_sq(move),
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

    int prom = prom_type(move);
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

bool Position::legal_move(Move move) const
{
    int from = from_sq(move);
    int ksq = this->position_of(KING, US);
    if (move & ENPASSANT)
    {
        u64 to_bb = BB(this->ep_sq);
        u64 cap_bb = to_bb >> 8;
        u64 pieces = (this->occupancy_bb() ^ BB(from) ^ cap_bb) | to_bb;

        u64 qr_bb = this->piece_bb(QUEEN) | this->piece_bb(ROOK);
        u64 qb_bb = this->piece_bb(QUEEN) | this->piece_bb(BISHOP);
        u64 them_bb = this->color_bb(THEM);

        return !(lookups::rook(ksq, pieces) & (qr_bb & them_bb))
            && !(lookups::bishop(ksq, pieces) & (qb_bb & them_bb));
    }
    else if (from == ksq)
    {
        return (move & CASTLING) || !(this->attackers_to(to_sq(move), THEM));
    }
    else
    {
        return !(this->pinned(US) & BB(from))
             || (BB(to_sq(move)) & lookups::full_ray(from, ksq));
    }
}

void Position::make_null_move()
{
    this->inc_half_moves();
    this->ep_sq = INVALID_SQ;
    this->flip();
    this->prev_hash_keys.push_back(this->hash_key);
    this->hash_key = this->calc_hash();
}

void Position::make_move(Move move)
{
    int from = from_sq(move),
        to = to_sq(move);

    this->castling_rights &= castling::spoilers[from]
                           & castling::spoilers[to];

    if (this->ep_sq != INVALID_SQ)
        this->ep_sq = INVALID_SQ;

    if (this->check_piece_on(from, PAWN))
    {
        this->reset_half_moves();
        this->clear_prev_hash_keys();
    }
    else
    {
        this->prev_hash_keys.push_back(this->hash_key);
        this->inc_half_moves();
    }

    switch (move & MOVE_TYPE_MASK) {
        case NORMAL:
            this->move_piece(from, to, this->piece_on(from), US);
            break;
        case CAPTURE:
            this->remove_piece(to, this->piece_on(to), THEM);
            this->move_piece(from, to, this->piece_on(from), US);
            this->reset_half_moves();
            this->clear_prev_hash_keys();
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
            int rfrom, rto;
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

    this->flip();
    this->hash_key = this->calc_hash();
}
