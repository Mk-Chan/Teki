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
#include "lookups.h"

char piece_char(int pt, int c)
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
    return c == US ? std::toupper(pchar) : pchar;
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

int Position::piece_on(int sq) const
{
    u64 sq_bb = BB(sq);
    for (int pt = PAWN; pt < NUM_PIECE_TYPES; ++pt)
        if (this->bb[pt] & sq_bb)
            return pt;
    return NO_PIECE;
}

void Position::display()
{
    for (int sq = A1; sq < NUM_SQUARES; ++sq) {
        if (sq && !(sq & 7))
            std::cout << '\n';
        int piece = this->piece_on(sq ^ 56);
        if (piece == NO_PIECE)
        {
            std::cout << "- ";
        }
        else
        {
            int color = ((this->color_bb(US) & BB(sq ^ 56)) && !this->is_flipped())
                     || ((this->color_bb(THEM) & BB(sq ^ 56)) && this->is_flipped())
                      ? US
                      : THEM;
            std::cout << piece_char(piece, color) << " ";
        }
    }
    std::cout << "\n"
              << "Zobrist key: " << this->hash_key
              << std::endl;
}

void Position::clear()
{
    for (int i = 0; i < 6; ++i)
        this->bb[i] = 0;
    for (int i = 0; i < 2; ++i)
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
    for (int i = 0 ; i < 64; ++i)
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
    int index = 0;
    for (int i = 0; i < 64;) {
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
            int sq = i ^ 56;
            int pt, pc;
            switch (c) {
            case 'p': pt = PAWN, pc = THEM; break;
            case 'r': pt = ROOK, pc = THEM; break;
            case 'n': pt = KNIGHT, pc = THEM; break;
            case 'b': pt = BISHOP, pc = THEM; break;
            case 'q': pt = QUEEN, pc = THEM; break;
            case 'k': pt = KING, pc = THEM; break;
            case 'P': pt = PAWN, pc = US; break;
            case 'R': pt = ROOK, pc = US; break;
            case 'N': pt = KNIGHT, pc = US; break;
            case 'B': pt = BISHOP, pc = US; break;
            case 'Q': pt = QUEEN, pc = US; break;
            case 'K': pt = KING, pc = US; break;
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
    for (char c : part) {
        if (c == '-')
        {
            break;
        }
        else if (!castling::is_frc)
        {
            switch (c) {
            case 'K': this->castling_rights |= US_OO; break;
            case 'Q': this->castling_rights |= US_OOO; break;
            case 'k': this->castling_rights |= THEM_OO; break;
            case 'q': this->castling_rights |= THEM_OOO; break;
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
    int half_moves;
    stream >> half_moves;
    this->half_moves = half_moves;

    // Fullmove number
    int full_moves; // dummy
    stream >> full_moves;

    if (need_to_flip)
        this->flip();

    this->hash_key = this->calc_hash();
}

u64 Position::attackers_to(int sq) const
{
    return this->attackers_to(sq, this->occupancy_bb());
}

u64 Position::attackers_to(int sq, int by_side) const
{
    return this->attackers_to(sq) & this->color_bb(by_side);
}

u64 Position::attackers_to(int sq, u64 occupancy) const
{
    return (lookups::rook(sq, occupancy) & (piece_bb(ROOK) | piece_bb(QUEEN)))
         | (lookups::bishop(sq, occupancy) & (piece_bb(BISHOP) | piece_bb(QUEEN)))
         | (lookups::knight(sq) & piece_bb(KNIGHT))
         | (lookups::pawn(sq, US) & piece_bb(PAWN, THEM))
         | (lookups::pawn(sq, THEM) & piece_bb(PAWN, US))
         | (lookups::king(sq) & piece_bb(KING));
}

u64 Position::attackers_to(int sq, int by_side, u64 occupancy) const
{
    return this->attackers_to(sq, occupancy) & this->color_bb(by_side);
}

u64 Position::checkers_to(int side) const
{
        return attackers_to(this->position_of(KING, side), !side);
}

u64 Position::calc_hash()
{
    bool flipped = this->is_flipped();
    if (flipped)
        this->flip();

    u64 hash_key = u64(0);
    for (int c = US; c <= THEM; ++c) {
        for (int pt = PAWN; pt <= KING; ++pt) {
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

u64 Position::pinned(int side) const
{
    int ksq = this->position_of(KING, side);
    u64 qr_bb = this->piece_bb(QUEEN) | this->piece_bb(ROOK);
    u64 qb_bb = this->piece_bb(QUEEN) | this->piece_bb(BISHOP);
    u64 nside_bb = this->color_bb(!side);

    u64 pinners = (qr_bb & nside_bb & lookups::rook(ksq))
                | (qb_bb & nside_bb & lookups::bishop(ksq));
    u64 occupancy_bb = this->occupancy_bb();

    u64 pinned = 0;
    while (pinners) {
        int sq = fbitscan(pinners);
        pinners &= pinners - 1;
        u64 bb = lookups::intervening_sqs(sq, ksq) & occupancy_bb;
        if (!(bb & (bb - 1)))
            pinned ^= bb & this->color_bb(side);
    }

    return pinned;
}

bool Position::is_passed_pawn(int sq) const
{
    return this->piece_on(sq) == PAWN
        && !(lookups::passed_pawn_mask(sq) & this->piece_bb(PAWN, THEM))
        && !(lookups::north(sq) & this->piece_bb(PAWN, US));
}

u64 Position::perft(int depth, bool root) const
{
    if (depth == 0)
        return u64(1);

    bool in_check = this->checkers_to(US);
    std::vector<Move> mlist;
    if (in_check)
        this->generate_in_check_movelist(mlist);
    else
        this->generate_movelist(mlist);

    u64 leaves = u64(0);
    for (Move move : mlist) {
        Position pos = *this;
        if (!pos.legal_move(move))
            continue;
        pos.make_move(move);
        u64 count = pos.perft(depth - 1, false);
        leaves += count;
        if (root)
            std::cout << get_move_string(move, !pos.is_flipped()) << ": "
                      << count << std::endl;
    }

    return leaves;
}

bool Position::is_repetition() const
{
    assert(this->prev_hash_keys.size() == this->get_half_moves());
    u64 curr_hash = this->get_hash_key();
    int num_keys = this->prev_hash_keys.size();
    for (int i = num_keys - 2; i >= num_keys - this->get_half_moves(); i -= 2)
        if (prev_hash_keys[i] == curr_hash)
            return true;
    return false;
}
