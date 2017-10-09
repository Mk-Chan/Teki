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

#ifndef SCORE_H
#define SCORE_H

#include "definitions.h"

class Score
{
public:
    // Constructors
    Score(i32 mg, i32 eg)
    {
        this->mg = mg;
        this->eg = eg;
    }

    // Operator overloads
    Score& operator+=(const Score& rhs)
    {
        this->mg += rhs.get_mg();
        this->eg += rhs.get_eg();
        return *this;
    }

    // Set values
    void set_mg(i32 mg) { this->mg = mg; }
    void set_eg(i32 eg) { this->eg = eg; }

    // Get values
    i32 get_mg() const { return mg; }
    i32 get_eg() const { return eg; }

private:
    i32 mg;
    i32 eg;
};

#endif
