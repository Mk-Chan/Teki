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

#include "tt.h"
#include "options.h"
#include "position.h"
#include "definitions.h"
#include "syzygy/tbprobe.h"

void syzygy_path_handler(std::string& s)
{
    tb_init(s.c_str());
    std::cout << "info string Largest tablebase size = " << TB_LARGEST
              << std::endl;
}

namespace options
{
    std::unordered_map<std::string, SpinOption> spins {
        { "Hash", { 1, 1, 1048576, [](int s) { tt.resize(s); } } },
        { "Threads", { 1, 1, MAX_THREADS, nullptr } },
        { "Contempt", { 20, -100, 100, nullptr } }
    };
    std::unordered_map<std::string, CheckOption> checks {
        { "UCI_Chess960", { castling::is_frc, [](bool b) { castling::is_frc = b; } } },
        { "Ponder", { allow_ponder, [](bool b) { allow_ponder = b; } } },
        { "MCTS", { mcts, [](bool b) { mcts = b; } } }
    };
    std::unordered_map<std::string, StringOption> strings {
        { "SyzygyPath", { "None", { [](std::string s) { syzygy_path_handler(s); } } } }
    };
}
