/*
 * Teki-MCTS
 * Copyright (C) 2018  Manik Charan
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "utils.h"

struct Controller
{
    std::uint64_t nodes_searched;
    std::uint64_t max_nodes;
    time_ms start_time;
    time_ms end_time;
    volatile bool time_dependent;
    volatile bool analyzing;
    volatile bool stop_search;
    bool limited_search;
    int max_ply;
    std::vector<uint32_t> search_moves;
};

inline Controller controller;

inline bool stopped()
{
    if (controller.stop_search)
        return true;

    if (controller.time_dependent)
    {
        time_ms curr_time = utils::curr_time();
        if (curr_time >= controller.end_time)
            return true;
        if (controller.end_time - curr_time <= 10)
            return true;
    }
    else if (controller.nodes_searched >= controller.max_nodes)
    {
        return true;
    }

    return false;
}

#endif
