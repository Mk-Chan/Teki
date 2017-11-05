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

int piece_value[5] = { 100, 300, 300, 500, 900 };

int eval_piece_values(const Position& pos)
{
    int value = 0;
    for (u32 pt = PAWN; pt < KING; ++pt)
        value += popcnt(pos.piece_bb(pt, US)) * piece_value[pt];
    return value;
}

int Position::evaluate()
{
    int eval = 0;
    for (u32 side = US; side <= THEM; ++side) {
        eval += eval_piece_values(*this);

        this->flip();
        eval = -eval;
    }
    return eval;
}
