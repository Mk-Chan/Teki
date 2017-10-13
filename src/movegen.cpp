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
#include "bitboard.h"

static inline void add_move(u32 move, std::vector<u32>& mlist)
{
    mlist.push_back(move);
}

static inline void extract_quiets(u32 from, u64 bb, std::vector<u32>& mlist)
{
    while (bb) {
        add_move(get_move(from, fbitscan(bb), NORMAL), mlist);
        bb &= bb - 1;
    }
}

static inline void extract_captures(Position& pos, u32 from, u64 bb, std::vector<u32>& mlist)
{
    while (bb) {
        u32 to = fbitscan(bb);
        add_move(get_move(from, to, CAPTURE, pos.piece_on(to)), mlist);
        bb &= bb - 1;
    }
}

std::vector<u32> Position::get_movelist()
{
    std::vector<Move> mlist;
    mlist.reserve(218);

    {   // Piece captures
        u32 from;
        u64 occupancy = this->occupancy_bb(),
            them = this->color_bb(THEM);
        for (u32 pt = KING; pt >= KNIGHT; --pt) {
            u64 curr_pieces = this->piece_bb(pt, US);
            while (curr_pieces) {
                from = fbitscan(curr_pieces);
                curr_pieces &= curr_pieces - 1;
                extract_captures(*this, from, lookups::attacks(pt, from, occupancy) & them, mlist);
            }
        }
    }

    {   // Pawn Captures
        u32 from;
        u32 cap_pt;
        u64 caps1, caps2, prom_caps1, prom_caps2;

        u64 pawns = this->piece_bb(PAWN, US);
        if (this->ep_sq != INVALID_SQ) {
            u64 ep_poss = pawns & lookups::pawn(this->ep_sq, THEM);
            while (ep_poss) {
                from = fbitscan(ep_poss);
                ep_poss &= ep_poss - 1;
                add_move(get_move(from, ep_sq, ENPASSANT), mlist);
            }
        }

        caps1 = ((pawns & ~FILE_A_MASK) << 7) & this->color_bb(THEM);
        prom_caps1 = caps1 & RANK_8_MASK;
        caps1 ^= prom_caps1;

        caps2 = ((pawns & ~FILE_H_MASK) << 9) & this->color_bb(THEM);
        prom_caps2 = caps2 & RANK_8_MASK;
        caps2 ^= prom_caps2;

        int to;
        while (caps1) {
            to = fbitscan(caps1);
            caps1 &= caps1 - 1;
            add_move(get_move(to - 7, to, CAPTURE, this->piece_on(to)), mlist);
        }
        while (caps2) {
            to = fbitscan(caps2);
            caps2 &= caps2 - 1;
            add_move(get_move(to - 9, to, CAPTURE, this->piece_on(to)), mlist);
        }
        while (prom_caps1) {
            to = fbitscan(prom_caps1);
            prom_caps1 &= prom_caps1 - 1;
            cap_pt = this->piece_on(to);
            add_move(get_move(to - 7, to, PROM_CAPTURE, cap_pt, PROM_TO_QUEEN), mlist);
            add_move(get_move(to - 7, to, PROM_CAPTURE, cap_pt, PROM_TO_KNIGHT), mlist);
            add_move(get_move(to - 7, to, PROM_CAPTURE, cap_pt, PROM_TO_BISHOP), mlist);
            add_move(get_move(to - 7, to, PROM_CAPTURE, cap_pt, PROM_TO_ROOK), mlist);
        }
        while (prom_caps2) {
            to = fbitscan(prom_caps2);
            prom_caps2 &= prom_caps2 - 1;
            cap_pt = this->piece_on(to);
            add_move(get_move(to - 9, to, PROM_CAPTURE, cap_pt, PROM_TO_QUEEN), mlist);
            add_move(get_move(to - 9, to, PROM_CAPTURE, cap_pt, PROM_TO_KNIGHT), mlist);
            add_move(get_move(to - 9, to, PROM_CAPTURE, cap_pt, PROM_TO_BISHOP), mlist);
            add_move(get_move(to - 9, to, PROM_CAPTURE, cap_pt, PROM_TO_ROOK), mlist);
        }
    }

    {   // Quiet promotions
        u64 prom_destinations = ((this->piece_bb(PAWN, US) & RANK_7_MASK) << 8) & ~this->occupancy_bb();
        while (prom_destinations) {
            u32 to = fbitscan(prom_destinations);
            prom_destinations &= prom_destinations - 1;
            add_move(get_move(to - 8, to, PROMOTION, CAP_NONE, PROM_TO_QUEEN), mlist);
            add_move(get_move(to - 8, to, PROMOTION, CAP_NONE, PROM_TO_KNIGHT), mlist);
            add_move(get_move(to - 8, to, PROMOTION, CAP_NONE, PROM_TO_BISHOP), mlist);
            add_move(get_move(to - 8, to, PROMOTION, CAP_NONE, PROM_TO_ROOK), mlist);
        }
    }

    {   // Castling
        static u32 const castling_side[2] = { WHITE_OO, WHITE_OOO };
        static u32 const king_end_pos[2] = { G1, C1 };
        static u32 const rook_end_pos[2] = { F1, D1 };

        u32 ksq = this->position_of(KING, US);
        if (!this->attackers_to(ksq, THEM)) {
            for (u32 i = KINGSIDE; i <= QUEENSIDE; ++i) {
                if (castling_side[i] & this->castling_rights) {
                    u32 king_end_sq = king_end_pos[i],
                        rook_end_sq = rook_end_pos[i];
                    u32 rsq = castling::rook_sqs[i];
                    u64 occupancy = this->occupancy_bb() ^ BB(ksq) ^ BB(rsq);

                    assert(this->check_piece_on(rsq, ROOK));

                    u64 intermediate_sqs = lookups::intervening_sqs(rsq, rook_end_sq) | BB(rook_end_sq);
                    if (intermediate_sqs & occupancy)
                        continue;

                    intermediate_sqs = lookups::intervening_sqs(ksq, king_end_sq) | BB(king_end_sq);
                    if (intermediate_sqs & occupancy)
                        continue;

                    u32 can_castle = 1;
                    while (intermediate_sqs) {
                        u32 sq = fbitscan(intermediate_sqs);
                        intermediate_sqs &= intermediate_sqs - 1;
                        if (this->attackers_to(sq, THEM)) {
                            can_castle = 0;
                            break;
                        }
                    }

                    if (can_castle)
                        add_move(get_move(ksq, king_end_sq, CASTLING), mlist);
                }
            }
        }
    }

    {   // Piece quiets
        u64 occupancy = this->occupancy_bb();
        u64 vacancy = ~occupancy;
        for (u32 pt = KNIGHT; pt <= KING; ++pt) {
            u64 curr_pieces = this->piece_bb(pt, US);
            while (curr_pieces) {
                u32 from = fbitscan(curr_pieces);
                curr_pieces &= curr_pieces - 1;
                extract_quiets(from, lookups::attacks(pt, from, occupancy) & vacancy, mlist);
            }
        }
    }

    {   // Pawn quiets
        u64 vacancy = ~this->occupancy_bb();
        u64 single_pushes_bb = ((this->piece_bb(PAWN, US) & ~RANK_7_MASK) << 8) & vacancy;
        u64 double_pushes_bb = ((single_pushes_bb & RANK_3_MASK) << 8) & vacancy;
        while (single_pushes_bb) {
            u32 to = fbitscan(single_pushes_bb);
            single_pushes_bb &= single_pushes_bb - 1;
            add_move(get_move(to - 8, to, NORMAL), mlist);
        }
        while (double_pushes_bb) {
            u32 to = fbitscan(double_pushes_bb);
            double_pushes_bb &= double_pushes_bb - 1;
            add_move(get_move(to - 16, to, DOUBLE_PUSH), mlist);
        }
    }

    return mlist;
}
