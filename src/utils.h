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

#ifndef UTILS_H
#define UTILS_H

#include <chrono>
#include <random>

using std::chrono::milliseconds;

typedef long time_ms;

namespace utils
{
    inline time_ms curr_time()
    {
        return std::chrono::duration_cast< milliseconds >(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }

    template<typename T>
    inline T random_num(T low_incl, T high_incl)
    {
        static std::mt19937_64 rng(88349201835);
        static std::uniform_int_distribution<T> uni(low_incl, high_incl);
        return uni(rng);
    }
}

#endif
