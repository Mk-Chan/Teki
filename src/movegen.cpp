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

static inline void add_move(i32 move, std::vector<Move>& mlist)
{
    mlist.push_back(move);
}

static inline void extract_quiets(i32 from, u64 bb, std::vector<Move>& mlist)
{
    while (bb) {
        add_move(get_move(from, fbitscan(bb), NORMAL), mlist);
        bb &= bb - 1;
    }
}

static inline void extract_captures(const Position& pos, i32 from, u64 bb, std::vector<Move>& mlist)
{
    while (bb) {
        i32 to = fbitscan(bb);
        add_move(get_move(from, to, CAPTURE, pos.piece_on(to)), mlist);
        bb &= bb - 1;
    }
}

void gen_piece_captures(const Position& pos, std::vector<Move>& mlist)
{
    i32 from;
    u64 occupancy = pos.occupancy_bb(),
        them = pos.color_bb(THEM);
    for (i32 pt = KING; pt >= KNIGHT; --pt) {
        u64 curr_pieces = pos.piece_bb(pt, US);
        while (curr_pieces) {
            from = fbitscan(curr_pieces);
            curr_pieces &= curr_pieces - 1;
            extract_captures(pos, from, lookups::attacks(pt, from, occupancy) & them, mlist);
        }
    }
}

void gen_pawn_captures(const Position& pos, std::vector<Move>& mlist) {
    i32 from;
    i32 cap_pt;
    u64 caps1, caps2, prom_caps1, prom_caps2;

    u64 pawns = pos.piece_bb(PAWN, US);
    if (pos.get_ep_sq() != INVALID_SQ) {
        u64 ep_poss = pawns & lookups::pawn(pos.get_ep_sq(), THEM);
        while (ep_poss) {
            from = fbitscan(ep_poss);
            ep_poss &= ep_poss - 1;
            add_move(get_move(from, pos.get_ep_sq(), ENPASSANT), mlist);
        }
    }

    caps1 = ((pawns & ~FILE_A_MASK) << 7) & pos.color_bb(THEM);
    prom_caps1 = caps1 & RANK_8_MASK;
    caps1 ^= prom_caps1;

    caps2 = ((pawns & ~FILE_H_MASK) << 9) & pos.color_bb(THEM);
    prom_caps2 = caps2 & RANK_8_MASK;
    caps2 ^= prom_caps2;

    int to;
    while (caps1) {
        to = fbitscan(caps1);
        caps1 &= caps1 - 1;
        add_move(get_move(to - 7, to, CAPTURE, pos.piece_on(to)), mlist);
    }
    while (caps2) {
        to = fbitscan(caps2);
        caps2 &= caps2 - 1;
        add_move(get_move(to - 9, to, CAPTURE, pos.piece_on(to)), mlist);
    }
    while (prom_caps1) {
        to = fbitscan(prom_caps1);
        prom_caps1 &= prom_caps1 - 1;
        cap_pt = pos.piece_on(to);
        add_move(get_move(to - 7, to, PROM_CAPTURE, cap_pt, PROM_TO_QUEEN), mlist);
        add_move(get_move(to - 7, to, PROM_CAPTURE, cap_pt, PROM_TO_KNIGHT), mlist);
        add_move(get_move(to - 7, to, PROM_CAPTURE, cap_pt, PROM_TO_BISHOP), mlist);
        add_move(get_move(to - 7, to, PROM_CAPTURE, cap_pt, PROM_TO_ROOK), mlist);
    }
    while (prom_caps2) {
        to = fbitscan(prom_caps2);
        prom_caps2 &= prom_caps2 - 1;
        cap_pt = pos.piece_on(to);
        add_move(get_move(to - 9, to, PROM_CAPTURE, cap_pt, PROM_TO_QUEEN), mlist);
        add_move(get_move(to - 9, to, PROM_CAPTURE, cap_pt, PROM_TO_KNIGHT), mlist);
        add_move(get_move(to - 9, to, PROM_CAPTURE, cap_pt, PROM_TO_BISHOP), mlist);
        add_move(get_move(to - 9, to, PROM_CAPTURE, cap_pt, PROM_TO_ROOK), mlist);
    }
}

void gen_quiet_promotions(const Position& pos, std::vector<Move>& mlist)
{
    u64 prom_destinations = ((pos.piece_bb(PAWN, US) & RANK_7_MASK) << 8) & ~pos.occupancy_bb();
    while (prom_destinations) {
        i32 to = fbitscan(prom_destinations);
        prom_destinations &= prom_destinations - 1;
        add_move(get_move(to - 8, to, PROMOTION, CAP_NONE, PROM_TO_QUEEN), mlist);
        add_move(get_move(to - 8, to, PROMOTION, CAP_NONE, PROM_TO_KNIGHT), mlist);
        add_move(get_move(to - 8, to, PROMOTION, CAP_NONE, PROM_TO_BISHOP), mlist);
        add_move(get_move(to - 8, to, PROMOTION, CAP_NONE, PROM_TO_ROOK), mlist);
    }
}

void gen_castling(const Position& pos, std::vector<Move>& mlist)
{
    static i32 const castling_side[2] = { WHITE_OO, WHITE_OOO };
    static i32 const king_end_pos[2] = { G1, C1 };
    static i32 const rook_end_pos[2] = { F1, D1 };

    if (!pos.checkers_to(US)) {
        i32 ksq = pos.position_of(KING, US);
        for (i32 i = KINGSIDE; i <= QUEENSIDE; ++i) {
            if (castling_side[i] & pos.get_castling_rights()) {
                i32 king_end_sq = king_end_pos[i],
                    rook_end_sq = rook_end_pos[i];
                i32 rsq = castling::rook_sqs[i];
                u64 occupancy = pos.occupancy_bb() ^ BB(ksq) ^ BB(rsq);

                assert(pos.check_piece_on(rsq, ROOK));

                u64 intermediate_sqs = lookups::intervening_sqs(rsq, rook_end_sq) | BB(rook_end_sq);
                if (intermediate_sqs & occupancy)
                    continue;

                intermediate_sqs = lookups::intervening_sqs(ksq, king_end_sq) | BB(king_end_sq);
                if (intermediate_sqs & occupancy)
                    continue;

                bool can_castle = true;
                while (intermediate_sqs) {
                    i32 sq = fbitscan(intermediate_sqs);
                    intermediate_sqs &= intermediate_sqs - 1;
                    if (pos.attackers_to(sq, THEM)) {
                        can_castle = false;
                        break;
                    }
                }

                if (can_castle)
                    add_move(get_move(ksq, king_end_sq, CASTLING), mlist);
            }
        }
    }
}

void gen_piece_quiets(const Position& pos, std::vector<Move>& mlist)
{
    u64 occupancy = pos.occupancy_bb();
    u64 vacancy = ~occupancy;
    for (i32 pt = KNIGHT; pt <= KING; ++pt) {
        u64 curr_pieces = pos.piece_bb(pt, US);
        while (curr_pieces) {
            i32 from = fbitscan(curr_pieces);
            curr_pieces &= curr_pieces - 1;
            extract_quiets(from, lookups::attacks(pt, from, occupancy) & vacancy, mlist);
        }
    }
}

void gen_pawn_quiets(const Position& pos, std::vector<Move>& mlist)
{
    u64 vacancy = ~pos.occupancy_bb();
    u64 single_pushes_bb = ((pos.piece_bb(PAWN, US) & ~RANK_7_MASK) << 8) & vacancy;
    u64 double_pushes_bb = ((single_pushes_bb & RANK_3_MASK) << 8) & vacancy;
    while (single_pushes_bb) {
        i32 to = fbitscan(single_pushes_bb);
        single_pushes_bb &= single_pushes_bb - 1;
        add_move(get_move(to - 8, to, NORMAL), mlist);
    }
    while (double_pushes_bb) {
        i32 to = fbitscan(double_pushes_bb);
        double_pushes_bb &= double_pushes_bb - 1;
        add_move(get_move(to - 16, to, DOUBLE_PUSH), mlist);
    }
}

void gen_checker_captures(const Position& pos, u64 checkers, std::vector<Move>& mlist)
{
    u64 our_pawns = pos.piece_bb(PAWN, US);
    u64 non_king_mask = ~pos.piece_bb(KING);
    u64 occupancy = pos.occupancy_bb();
    i32 ep_sq = pos.get_ep_sq();

    if (ep_sq != INVALID_SQ && ((BB(ep_sq) >> 8) & checkers))
    {
        u64 enpassanters = our_pawns & lookups::pawn(ep_sq, THEM);
        while (enpassanters) {
            int attacker_sq = fbitscan(enpassanters);
            enpassanters &= enpassanters - 1;
            add_move(get_move(attacker_sq, ep_sq, ENPASSANT), mlist);
        }
    }

    while (checkers) {
        int checker_sq = fbitscan(checkers);
        int checker_pt = pos.piece_on(checker_sq);
        checkers &= checkers - 1;
        u64 attackers = pos.attackers_to(checker_sq, US, occupancy) & non_king_mask;
        while (attackers) {
            int attacker_sq = fbitscan(attackers);
            attackers &= attackers - 1;
            if (   (BB(attacker_sq) & our_pawns)
                && (BB(checker_sq) & RANK_8_MASK))
            {
                add_move(get_move(attacker_sq, checker_sq, PROM_CAPTURE,
                                  checker_pt, PROM_TO_QUEEN), mlist);
                add_move(get_move(attacker_sq, checker_sq, PROM_CAPTURE,
                                  checker_pt, PROM_TO_KNIGHT), mlist);
                add_move(get_move(attacker_sq, checker_sq, PROM_CAPTURE,
                                  checker_pt, PROM_TO_ROOK), mlist);
                add_move(get_move(attacker_sq, checker_sq, PROM_CAPTURE,
                                  checker_pt, PROM_TO_BISHOP), mlist);
            }
            else
            {
                add_move(get_move(attacker_sq, checker_sq, CAPTURE, checker_pt), mlist);
            }
        }
    }
}

void gen_check_blocks(const Position& pos, u64 blocking_possibilites, std::vector<Move>& mlist)
{
    u64 our_pawns = pos.piece_bb(PAWN, US);
    u64 inclusion_mask = ~(our_pawns | pos.piece_bb(KING) | pos.pinned(US));
    u64 occupancy = pos.occupancy_bb();
    u64 vacancy_mask = ~occupancy;

    while (blocking_possibilites) {
        int blocking_sq = fbitscan(blocking_possibilites);
        blocking_possibilites &= blocking_possibilites - 1;
        u64 pawn_blockers = BB(blocking_sq) >> 8;
        if (pawn_blockers & our_pawns)
        {
            int blocker_sq = fbitscan(pawn_blockers);
            if (BB(blocking_sq) & RANK_8_MASK)
            {
                add_move(get_move(blocker_sq, blocking_sq, PROMOTION,
                                  CAP_NONE, PROM_TO_QUEEN), mlist);
                add_move(get_move(blocker_sq, blocking_sq, PROMOTION,
                                  CAP_NONE, PROM_TO_KNIGHT), mlist);
                add_move(get_move(blocker_sq, blocking_sq, PROMOTION,
                                  CAP_NONE, PROM_TO_ROOK), mlist);
                add_move(get_move(blocker_sq, blocking_sq, PROMOTION,
                                  CAP_NONE, PROM_TO_BISHOP), mlist);
            }
            else
            {
                add_move(get_move(blocker_sq, blocking_sq, NORMAL), mlist);
            }
        }
        else if(   (rank_of(blocking_sq) == RANK_4)
                && (pawn_blockers & vacancy_mask)
                && (pawn_blockers = (pawn_blockers >> 8) & our_pawns))
        {
            add_move(get_move(fbitscan(pawn_blockers), blocking_sq,
                              DOUBLE_PUSH), mlist);
        }

        u64 candidate_blockers = pos.attackers_to(blocking_sq, US, occupancy)
                               & inclusion_mask;
        while (candidate_blockers) {
            add_move(get_move(fbitscan(candidate_blockers), blocking_sq,
                              NORMAL), mlist);
            candidate_blockers &= candidate_blockers - 1;
        }
    }
}

void Position::generate_in_check_movelist(std::vector<Move>& mlist) const
{
    i32 ksq = this->position_of(KING, US);
    u64 checkers = this->checkers_to(US);
    u64 evasions = lookups::king(ksq) & ~this->color_bb(US);

    u64 occupancy = this->occupancy_bb();
    u64 sans_king = occupancy ^ BB(ksq);

    while (evasions) {
        int sq = fbitscan(evasions);
        evasions &= evasions - 1;
        if (!this->attackers_to(sq, THEM, sans_king)) {
            if (occupancy & BB(sq))
                add_move(get_move(ksq, sq, CAPTURE, this->piece_on(sq)), mlist);
            else
                add_move(get_move(ksq, sq, NORMAL), mlist);
        }
    }

    if (checkers & (checkers - 1))
        return;

    gen_checker_captures(*this, checkers, mlist);

    if (checkers & lookups::king(ksq))
        return;

    u64 blocking_possibilites = lookups::intervening_sqs(fbitscan(checkers), ksq);
    if (blocking_possibilites)
        gen_check_blocks(*this, blocking_possibilites, mlist);
}

void Position::generate_quiesce_movelist(std::vector<Move>& mlist) const
{
    const Position& pos = *this;
    gen_piece_captures(pos, mlist);
    gen_pawn_captures(pos, mlist);
    gen_quiet_promotions(pos, mlist);
}

void Position::generate_movelist(std::vector<Move>& mlist) const
{
    const Position& pos = *this;
    gen_piece_captures(pos, mlist);
    gen_pawn_captures(pos, mlist);
    gen_quiet_promotions(pos, mlist);
    gen_castling(pos, mlist);
    gen_piece_quiets(pos, mlist);
    gen_pawn_quiets(pos, mlist);
}
