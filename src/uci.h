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

#ifndef UCI_H
#define UCI_H

#ifndef NAME
#define NAME ("Teki")
#endif

#define AUTHOR ("Manik Charan")

#include "move.h"
#include "utils.h"

namespace uci
{
    extern void init();
    extern void print_currmove(Move move, i32 move_num, time_ms start_time, bool flipped);
    extern void print_search(int score, int depth, time_ms time,
                             std::vector<Move>& pv, bool flipped);
}

#endif
