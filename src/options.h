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

#ifndef OPTIONS_H
#define OPTIONS_H

#include <string>
#include <functional>
#include <unordered_map>

struct SpinOption
{
    SpinOption() {}
    SpinOption(int value, int min, int max, std::function<void(int)> handler)
        : value(value), min(min), max(max), handler(handler) {}

    void setoption(int value)
    {
        if (value >= min && value <= max)
        {
            this->value = value;
            handler(value);
        }
    }

    int value;
    int min;
    int max;
    std::function<void(int)> handler;
};


namespace options
{
    extern std::unordered_map<std::string, SpinOption> spins;
}

#endif
