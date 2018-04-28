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

#ifndef EVALUATE_H
#define EVALUATE_H

#include "score.h"

inline int piece_phase[5] = { 1, 10, 10, 20, 40 };
inline Score piece_value[5] = {
    S(100, 100), S(400, 300), S(400, 300), S(600, 500), S(1200, 900)
};

inline Score passed_pawn[4][7] = {
    // Pawn cannot advance
    { 0, S(4, 4), S(8, 8), S(12, 18), S(27, 35), S(75, 110), S(100, 220) },
    // Pawn can advance, destination is attacked and protected
    { 0, S(5, 5), S(10, 10), S(15, 20), S(30, 40), S(80, 120), S(120, 250) },
    // Pawn can advance, destination square is attacked and protected
    { 0, S(7, 7), S(12, 12), S(17, 22), S(35, 45), S(90, 160), S(130, 300) },
    // Pawn can advance, destination square is not attacked
    { 0, S(7, 7), S(15, 15), S(20, 25), S(40, 50), S(100, 200), S(150, 400) }
};
inline Score doubled_pawns = S(-10, -10);
inline Score isolated_pawn = S(-10, -10);

inline Score rook_on_7th_rank = S(40, 20);
inline Score bishop_pair = S(50, 80);

inline Score piece_mobility[5][28] = {
    {},
    {   // Knight
        S(-50, -50), S(-30, -30), S(-10, -10), S(0, 0), S(10, 10), S(20, 20),
        S(25, 25), S(30, 30), S(35, 35)
    },
    {   // Bishop
        S(-40, -40), S(-20, -20), S(0, 0), S(10, 10), S(20, 20), S(25, 25),
        S(30, 30), S(30, 30), S(35, 35), S(35, 35), S(40, 40), S(40, 40),
        S(45, 45), S(45, 45)
    },
    {   // Rook
        S(-30, -30), S(-20, -20), S(-10, -10), S(0, 0), S(5, 10), S(10, 20),
        S(15, 30), S(20, 40), S(25, 50), S(30, 55), S(35, 60), S(40, 65),
        S(45, 70), S(50, 75), S(55, 80)
    },
    {   // Queen
        S(-30, -30), S(-20, -20), S(-10, -10), S(0, 0), S(5, 5), S(10, 10),
        S(15, 15), S(20, 20), S(25, 25), S(30, 30), S(35, 35), S(40, 40),
        S(45, 45), S(50, 50), S(55, 55), S(60, 60), S(60, 65), S(60, 70),
        S(60, 75), S(60, 80), S(60, 85), S(60, 90), S(60, 90), S(60, 90),
        S(60, 90), S(60, 90), S(60, 90), S(60, 90)
    }
};

inline Score close_shelter = S(30, 5);
inline Score far_shelter = S(20, 5);
inline int king_attack_weight[5] = { 0, 3, 3, 4, 5 };
inline int king_attack_table[100] = { // Taken from CPW(Glaurung 1.2)
      0,   0,   0,   1,   1,   2,   3,   4,   5,   6,
      8,  10,  13,  16,  20,  25,  30,  36,  42,  48,
     55,  62,  70,  80,  90, 100, 110, 120, 130, 140,
    150, 160, 170, 180, 190, 200, 210, 220, 230, 240,
    250, 260, 270, 280, 290, 300, 310, 320, 330, 340,
    350, 360, 370, 380, 390, 400, 410, 420, 430, 440,
    450, 460, 470, 480, 490, 500, 510, 520, 530, 540,
    550, 560, 570, 580, 590, 600, 610, 620, 630, 640,
    650, 650, 650, 650, 650, 650, 650, 650, 650, 650,
    650, 650, 650, 650, 650, 650, 650, 650, 650, 650
};

inline Score psqt[6][64];
inline Score psq_tmp[6][64] = {
    {   // Pawn
        S(  0,   0), S(  0,   0), S(  0,   0), S(  0,   0),
        S(  0,   0), S(  0,   0), S(  0,   0), S(  0,   0),
        S(  0,   5), S(  0,   5), S(  0,   5), S( 10,   5),
        S(  0,  10), S(  0,  10), S(  0,  10), S( 20,  10),
        S(  0,  30), S(  0,  30), S(  0,  30), S( 10,  30),
        S(  0,  50), S(  0,  50), S(  0,  50), S(  0,  50),
        S(  0,  80), S(  0,  80), S(  0,  80), S(  0,  80),
        S(  0,   0), S(  0,   0), S(  0,   0), S(  0,   0)
    },
    {   // Knight
        S(-50, -30), S(-30, -20), S(-20, -10), S(-15,   0),
        S(-30, -20), S( -5, -10), S(  0,  -5), S(  5,   5),
        S(-10, -10), S(  0,  -5), S(  5,   5), S( 10,  10),
        S(-10,   0), S(  5,   0), S( 10,  15), S( 20,  20),
        S(-10,   0), S(  5,   0), S( 10,  15), S( 20,  20),
        S(-10, -10), S(  0,  -5), S(  5,   5), S( 10,  10),
        S(-30, -20), S( -5, -10), S(  0,  -5), S(  5,   5),
        S(-50, -30), S(-30, -20), S(-20, -10), S(-15,   0)
    },
    {   // Bishop
        S(-20, -20), S(-20, -15), S(-20, -10), S(-20, -10),
        S(-10,   0), S(  0,   0), S( -5,   0), S(  0,   0),
        S( -5,   0), S(  5,   0), S(  5,   0), S(  5,   0),
        S(  0,   0), S(  5,   0), S( 10,   0), S( 15,   0),
        S(  0,   0), S(  5,   0), S( 10,   0), S( 15,   0),
        S( -5,   0), S(  5,   0), S(  5,   0), S(  5,   0),
        S(-10,   0), S(  0,   0), S( -5,   0), S(  0,   0),
        S(-20, -20), S(-20, -15), S(-20, -10), S(-20, -10)
    },
    {   // Rook
        S( -5,  -5), S(  0,  -3), S(  2,  -1), S(  5,   0),
        S( -5,   0), S(  0,   0), S(  2,   0), S(  5,   0),
        S( -5,   0), S(  0,   0), S(  2,   0), S(  5,   0),
        S( -5,   0), S(  0,   0), S(  2,   0), S(  5,   0),
        S( -5,   0), S(  0,   0), S(  2,   0), S(  5,   0),
        S( -5,   0), S(  0,   0), S(  2,   0), S(  5,   0),
        S( -5,   0), S(  0,   0), S(  2,   0), S(  5,   0),
        S( -5,  -5), S(  0,  -3), S(  2,  -1), S(  5,   0)
    },
    {
        // Queen
        S(-10, -20), S( -5, -10), S( -5,  -5), S( -5,   0),
        S( -5, -10), S(  0,  -5), S(  0,   0), S(  0,   5),
        S( -5,  -5), S(  0,   5), S(  0,   5), S(  0,  10),
        S( -5,   0), S(  0,   5), S(  0,  10), S(  0,  15),
        S( -5,   0), S(  0,   5), S(  0,  10), S(  0,  15),
        S( -5,  -5), S(  0,   5), S(  0,   5), S(  0,  10),
        S( -5, -10), S(  0,  -5), S(  0,   0), S(  0,   5),
        S(-10, -20), S( -5, -10), S( -5,  -5), S( -5,   0)
    },
    {
        // King
        S( 30, -70), S( 45, -45), S( 35, -35), S(-10, -20),
        S( 10, -40), S( 20, -25), S(  0, -10), S(-15,   5),
        S(-20, -30), S(-25, -15), S(-30,   5), S(-30,  10),
        S(-40, -20), S(-50,   5), S(-60,  10), S(-70,  20),
        S(-70, -20), S(-80,   5), S(-90,  10), S(-90,  20),
        S(-70, -30), S(-80, -15), S(-90,   5), S(-90,  10),
        S(-80, -40), S(-80, -25), S(-90, -10), S(-90,   0),
        S(-90, -70), S(-90, -45), S(-90, -15), S(-90, -20)
    }
};

namespace eval
{
    inline void init()
    {
        int k = 0;
        for (int i = 0; i < 8; ++i) {
            for (int j = 0;  j < 4; ++j) {
                int sq1 = get_sq(i, j);
                int sq2 = get_sq(i, (7 - j));
                for (int pt = PAWN; pt <= KING; ++pt) {
                    psqt[pt][sq1]
                        = psqt[pt][sq2]
                        = psq_tmp[pt][k];
                }
                ++k;
            }
        }
    }
}

#endif
