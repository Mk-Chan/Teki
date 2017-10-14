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

#include <random>
#include "position.h"
#include "move.h"

Move Position::best_move()
{
    // Return a random move for now
    std::vector<Move> mlist = this->get_movelist();
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> uni(0, mlist.size() - 1);

    Move move;
    while (true) {
        Position tmp = *this;
        u32 r = uni(rng);
        move = mlist[r];
        if (tmp.make_move(move))
            break;
    }

    return move;
}