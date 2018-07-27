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

#include "options.h"
#include "position.h"
#include "definitions.h"

namespace options
{
    std::unordered_map<std::string, SpinOption> spins {
        { "Threads", { 1, 1, MAX_THREADS, nullptr } },
    };
    std::unordered_map<std::string, CheckOption> checks {
        { "UCI_Chess960", { castling::is_frc, [](bool b) { castling::is_frc = b; } } },
        { "Ponder", { allow_ponder, [](bool b) { allow_ponder = b; } } },
    };
    std::unordered_map<std::string, StringOption> strings {
    };
}
