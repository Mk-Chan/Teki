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

#include <iostream>
#include "definitions.h"
#include "lookups.h"
#include "utils.h"

u64 psq_keys_bb[2][6][64];
u64 castle_keys_bb[16];
u64 ep_keys_bb[64];
u64 stm_key_bb;

int distance_val[64][64];
u64 ray_bb[64][64];
u64 xray_bb[64][64];
u64 full_ray_bb[64][64];
u64 intervening_ray_bb[64][64];
u64 adjacent_files_bb[64];
u64 adjacent_sqs_bb[64];
u64 file_mask_bb[64];
u64 rank_mask_bb[64];

u64 pawn_attacks[2][64];
u64 knight_attacks[64];
u64 bishop_attacks[64];
u64 rook_attacks[64];
u64 queen_attacks[64];
u64 king_attacks[64];

u64 north_bb[64];
u64 south_bb[64];
u64 east_bb[64];
u64 west_bb[64];
u64 northeast_bb[64];
u64 northwest_bb[64];
u64 southeast_bb[64];
u64 southwest_bb[64];

u64 passed_pawn_mask_bb[64];
u64 king_danger_zone_bb[64];
u64 king_shelter_mask_bb[64][2];

void print_bb(u64 bb)
{
    for (int sq = 0; sq < NUM_SQUARES; ++sq) {
        if (sq && !(sq & 7))
            std::cout << '\n';

        if (BB(sq ^ 56) & bb)
            std::cout << "X  ";
        else
            std::cout << "-  ";
    }
    std::cout << std::endl;
}

void init_non_sliders()
{
    for (int sq = A1; sq < NUM_SQUARES; ++sq) {
        king_attacks[sq] = knight_attacks[sq] = 0;
        pawn_attacks[US][sq] = pawn_attacks[THEM][sq] = 0;

        // Pawn attacks
        pawn_attacks[US][sq] |= ((sBB(sq + 7) & ~FILE_H_MASK)
                               | (sBB(sq + 9) & ~FILE_A_MASK))
                               & ~RANK_1_MASK;
        pawn_attacks[THEM][sq] |= ((sBB(sq - 7) & ~FILE_A_MASK)
                                 | (sBB(sq - 9) & ~FILE_H_MASK))
                                 & ~RANK_8_MASK;

        // Knight attacks
        knight_attacks[sq] |= sBB(sq + 17) & ~(FILE_A_MASK | RANK_2_MASK | RANK_1_MASK);
        knight_attacks[sq] |= sBB(sq + 15) & ~(FILE_H_MASK | RANK_2_MASK | RANK_1_MASK);
        knight_attacks[sq] |= sBB(sq - 17) & ~(FILE_H_MASK | RANK_7_MASK | RANK_8_MASK);
        knight_attacks[sq] |= sBB(sq - 15) & ~(FILE_A_MASK | RANK_7_MASK | RANK_8_MASK);
        knight_attacks[sq] |= sBB(sq - 10) & ~(FILE_H_MASK | FILE_G_MASK | RANK_8_MASK);
        knight_attacks[sq] |= sBB(sq + 6)  & ~(FILE_H_MASK | FILE_G_MASK | RANK_1_MASK);
        knight_attacks[sq] |= sBB(sq + 10) & ~(FILE_A_MASK | FILE_B_MASK | RANK_1_MASK);
        knight_attacks[sq] |= sBB(sq - 6)  & ~(FILE_A_MASK | FILE_B_MASK | RANK_8_MASK);

        // King attacks
        king_attacks[sq] |= sBB(sq + 8) & ~RANK_1_MASK;
        king_attacks[sq] |= sBB(sq - 8) & ~RANK_8_MASK;
        king_attacks[sq] |= sBB(sq + 1) & ~FILE_A_MASK;
        king_attacks[sq] |= sBB(sq - 1) & ~FILE_H_MASK;
        king_attacks[sq] |= sBB(sq + 9) & ~(FILE_A_MASK | RANK_1_MASK);
        king_attacks[sq] |= sBB(sq - 9) & ~(FILE_H_MASK | RANK_8_MASK);
        king_attacks[sq] |= sBB(sq + 7) & ~(FILE_H_MASK | RANK_1_MASK);
        king_attacks[sq] |= sBB(sq - 7) & ~(FILE_A_MASK | RANK_8_MASK);
    }
}

void init_pseudo_sliders()
{
    for (int sq = A1; sq < NUM_SQUARES; ++sq) {
        bishop_attacks[sq] = lookups::northeast(sq) | lookups::northwest(sq)
                           | lookups::southeast(sq) | lookups::southwest(sq);
        rook_attacks[sq] = lookups::north(sq) | lookups::south(sq)
                         | lookups::east(sq) | lookups::west(sq);
        queen_attacks[sq] = rook_attacks[sq] | bishop_attacks[sq];
    }
}

void init_misc()
{
    int i, j, high, low;
    for (i = 0; i < 64; i++) {
        file_mask_bb[i] = lookups::north(i) | lookups::south(i) | BB(i);
        rank_mask_bb[i] = lookups::east(i) | lookups::west(i) | BB(i);
        passed_pawn_mask_bb[i] = 0;
        if (file_of(i) != FILE_A)
        {
            passed_pawn_mask_bb[i] |= north_bb[i-1] | north_bb[i];
            adjacent_files_bb[i] |= BB(i-1) | north_bb[i-1] | south_bb[i-1];
            adjacent_sqs_bb[i] |= BB(i-1);
        }
        if (file_of(i) != FILE_H)
        {
            passed_pawn_mask_bb[i] |= north_bb[i+1] | north_bb[i];
            adjacent_files_bb[i] |= BB(i+1) | north_bb[i+1] | south_bb[i+1];
            adjacent_sqs_bb[i] |= BB(i+1);
        }
        for (j = 0; j < 64; j++) {
            distance_val[i][j] = std::max(abs(int(rank_of(i)) - int(rank_of(j))),
                                      abs(int(file_of(i)) - int(file_of(j))));
            intervening_ray_bb[i][j] = 0;
            if (i == j)
                continue;
            high = j;
            if (i > j) {
                high = i;
                low = j;
            }
            else
                low = i;
            if (file_of(high) == file_of(low))
            {
                full_ray_bb[i][j] =
                    (lookups::rook(high) & lookups::rook(low)) | BB(i) | BB(j);
                xray_bb[low][high] = lookups::north(low);
                xray_bb[high][low] = lookups::south(high);
                ray_bb[i][j] = BB(high);
                for (high -= 8; high >= low; high -= 8) {
                    ray_bb[i][j] |= BB(high);
                    if (high != low)
                        intervening_ray_bb[i][j] |= BB(high);
                }
            }
            else if (rank_of(high) == rank_of(low))
            {
                full_ray_bb[i][j] =
                    (lookups::rook(high) & lookups::rook(low)) | BB(i) | BB(j);
                xray_bb[low][high] = lookups::east(low);
                xray_bb[high][low] = lookups::west(high);
                ray_bb[i][j] = BB(high);
                for (--high; high >= low; high--) {
                    ray_bb[i][j] |= BB(high);
                    if (high != low)
                        intervening_ray_bb[i][j] |= BB(high);
                }
            }
            else if (rank_of(high) - rank_of(low) == file_of(high) - file_of(low))
            {
                full_ray_bb[i][j] =
                    (lookups::bishop(high) & lookups::bishop(low)) | BB(i) | BB(j);
                xray_bb[low][high] = lookups::northeast(low);
                xray_bb[high][low] = lookups::southwest(high);
                ray_bb[i][j] = BB(high);
                for (high -= 9; high >= low; high -= 9) {
                    ray_bb[i][j] |= BB(high);
                    if (high != low)
                        intervening_ray_bb[i][j] |= BB(high);
                }
            }
            else if (rank_of(high) - rank_of(low) == file_of(low) - file_of(high))
            {
                full_ray_bb[i][j] =
                    (lookups::bishop(high) & lookups::bishop(low)) | BB(i) | BB(j);
                xray_bb[low][high] = lookups::northwest(low);
                xray_bb[high][low] = lookups::southeast(high);
                ray_bb[i][j] = BB(high);
                for (high -= 7; high >= low; high -= 7) {
                    ray_bb[i][j] |= BB(high);
                    if (high != low)
                        intervening_ray_bb[i][j] |= BB(high);
                }
            }
        }
    }
}

void init_directions()
{
    u64 ne, nw, se, sw;
    u64 n, w, s, e;
    u64 ne_mask, nw_mask, se_mask, sw_mask;
    u64 n_mask, w_mask, s_mask, e_mask;
    nw_mask = ~(RANK_1_MASK | FILE_H_MASK);
    ne_mask = ~(RANK_1_MASK | FILE_A_MASK);
    sw_mask = ~(RANK_8_MASK | FILE_H_MASK);
    se_mask = ~(RANK_8_MASK | FILE_A_MASK);
    n_mask = ~(RANK_1_MASK);
    e_mask = ~(FILE_A_MASK);
    s_mask = ~(RANK_8_MASK);
    w_mask = ~(FILE_H_MASK);

    for (int sq = A1; sq < NUM_SQUARES; ++sq) {
        u64 sq_bb = BB(sq);

        nw = (sq_bb << 7) & nw_mask;
        ne = (sq_bb << 9) & ne_mask;
        sw = (sq_bb >> 9) & sw_mask;
        se = (sq_bb >> 7) & se_mask;
        n = (sq_bb << 8) & n_mask;
        e = (sq_bb << 1) & e_mask;
        s = (sq_bb >> 8) & s_mask;
        w = (sq_bb >> 1) & w_mask;

        for (int times = 0; times < 6; ++times) {
            nw |= (nw << 7) & nw_mask;
            ne |= (ne << 9) & ne_mask;
            sw |= (sw >> 9) & sw_mask;
            se |= (se >> 7) & se_mask;
            n |= (n << 8) & n_mask;
            e |= (e << 1) & e_mask;
            s |= (s >> 8) & s_mask;
            w |= (w >> 1) & w_mask;
        }

        north_bb[sq] = n;
        south_bb[sq] = s;
        east_bb[sq] = e;
        west_bb[sq] = w;
        northeast_bb[sq] = ne;
        northwest_bb[sq] = nw;
        southeast_bb[sq] = se;
        southwest_bb[sq] = sw;
    }
}

void init_keys()
{
    for (int c = US; c <= THEM; ++c) {
        for (int pt = PAWN; pt <= KING; ++pt) {
            for (int sq = A1; sq <= H8; ++sq) {
                psq_keys_bb[c][pt][sq] = utils::rand_u64(0, UINT64_MAX);
            }
        }
    }
    for (int sq = A1; sq <= H8; ++sq) {
        ep_keys_bb[sq] = utils::rand_u64(0, UINT64_MAX);
    }
    for (int cr = 0; cr < 16; ++cr) {
        castle_keys_bb[cr] = utils::rand_u64(0, UINT64_MAX);
    }
    stm_key_bb = utils::rand_u64(0, UINT64_MAX);
}

void init_eval_masks()
{
    for (int i = 0; i < 64; ++i) {
        king_danger_zone_bb[i] =
              BB(i)
            | lookups::king(i)
            | (lookups::king(i) >> 8);
        if (i < 56)
        {
            king_shelter_mask_bb[i][0] = sBB(i + 8)
                | (lookups::king(i) & (lookups::king(i+8) << 8));
            if (i < 48)
                king_shelter_mask_bb[i][1] = king_shelter_mask_bb[i][0] << 8;
        }
    }
}

namespace lookups
{
    void init()
    {
        init_non_sliders();
        init_directions();
        init_pseudo_sliders();
        init_misc();
        init_keys();
        init_eval_masks();
    }

    u64 psq_key(int c, int pt, int sq) { return psq_keys_bb[c][pt][sq]; }
    u64 castle_key(int rights) { return castle_keys_bb[rights]; }
    u64 ep_key(int sq) { return ep_keys_bb[sq]; }
    u64 stm_key() { return stm_key_bb; }

    int distance(int from, int to) { return distance_val[from][to]; }
    u64 ray(int from, int to) { return ray_bb[from][to]; }
    u64 xray(int from, int to) { return xray_bb[from][to]; }
    u64 full_ray(int from, int to) { return full_ray_bb[from][to]; }
    u64 intervening_sqs(int from, int to) { return intervening_ray_bb[from][to]; }
    u64 adjacent_files(int sq) { return adjacent_files_bb[sq]; }
    u64 adjacent_sqs(int sq) { return adjacent_sqs_bb[sq]; }
    u64 file_mask(int sq) { return file_mask_bb[sq]; }
    u64 rank_mask(int sq) { return rank_mask_bb[sq]; }

    u64 north(int square) { return north_bb[square]; }
    u64 south(int square) { return south_bb[square]; }
    u64 east(int square) { return east_bb[square]; }
    u64 west(int square) { return west_bb[square]; }
    u64 northeast(int square) { return northeast_bb[square]; }
    u64 northwest(int square) { return northwest_bb[square]; }
    u64 southeast(int square) { return southeast_bb[square]; }
    u64 southwest(int square) { return southwest_bb[square]; }

    u64 pawn(int square, int side) { return pawn_attacks[side][square]; }
    u64 knight(int square) { return knight_attacks[square]; }
    u64 bishop(int square) { return bishop_attacks[square]; }
    u64 rook(int square) { return rook_attacks[square]; }
    u64 queen(int square) { return queen_attacks[square]; }
    u64 king(int square) { return king_attacks[square]; }

    u64 bishop(int square, u64 occupancy)
    {
        u64 atk = bishop(square);
        u64 nw_blockers = (northwest(square) & occupancy) | BB(A8);
        u64 ne_blockers = (northeast(square) & occupancy) | BB(H8);
        u64 sw_blockers = (southwest(square) & occupancy) | BB(A1);
        u64 se_blockers = (southeast(square) & occupancy) | BB(H1);

        atk ^= northwest(fbitscan(nw_blockers));
        atk ^= northeast(fbitscan(ne_blockers));
        atk ^= southwest(rbitscan(sw_blockers));
        atk ^= southeast(rbitscan(se_blockers));

        return atk;
    }

    u64 rook(int square, u64 occupancy)
    {
        u64 atk = rook(square);
        u64 n_blockers = (north(square) & occupancy) | BB(H8);
        u64 s_blockers = (south(square) & occupancy) | BB(A1);
        u64 w_blockers = (west(square) & occupancy) | BB(A1);
        u64 e_blockers = (east(square) & occupancy) | BB(H8);

        atk ^= north(fbitscan(n_blockers));
        atk ^= south(rbitscan(s_blockers));
        atk ^= west(rbitscan(w_blockers));
        atk ^= east(fbitscan(e_blockers));

        return atk;
    }

    u64 queen(int square, u64 occupancy)
    {
        u64 atk = queen(square);
        u64 nw_blockers = (northwest(square) & occupancy) | BB(A8);
        u64 ne_blockers = (northeast(square) & occupancy) | BB(H8);
        u64 sw_blockers = (southwest(square) & occupancy) | BB(A1);
        u64 se_blockers = (southeast(square) & occupancy) | BB(H1);
        u64 n_blockers = (north(square) & occupancy) | BB(H8);
        u64 s_blockers = (south(square) & occupancy) | BB(A1);
        u64 w_blockers = (west(square) & occupancy) | BB(A1);
        u64 e_blockers = (east(square) & occupancy) | BB(H8);

        atk ^= northwest(fbitscan(nw_blockers));
        atk ^= northeast(fbitscan(ne_blockers));
        atk ^= southwest(rbitscan(sw_blockers));
        atk ^= southeast(rbitscan(se_blockers));
        atk ^= north(fbitscan(n_blockers));
        atk ^= south(rbitscan(s_blockers));
        atk ^= west(rbitscan(w_blockers));
        atk ^= east(fbitscan(e_blockers));

        return atk;
    }

    u64 attacks(int piece_type, int square, u64 occupancy, int side)
    {
        switch (piece_type) {
        case PAWN: return pawn(square, side);
        case KNIGHT: return knight(square);
        case BISHOP: return bishop(square, occupancy);
        case ROOK: return rook(square, occupancy);
        case QUEEN: return queen(square, occupancy);
        case KING: return king(square);
        default: return -1;
        }
    }

    u64 passed_pawn_mask(int square) { return passed_pawn_mask_bb[square]; }
    u64 king_danger_zone(int square) { return king_danger_zone_bb[square]; }
    std::pair<u64, u64> king_shelter_masks(int square)
    {
        return {
            king_shelter_mask_bb[square][0],
            king_shelter_mask_bb[square][1]
        };
    }
}
