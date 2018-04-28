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
    std::unordered_map<std::string, SpinOption> spins = {
        { "Hash", { 1, 1, 1048576, [](int s) { tt.resize(s); } } },
        { "Threads", { 1, 1, MAX_THREADS, nullptr } },
        { "Contempt", { 20, -100, 100, nullptr } }
    };
    std::unordered_map<std::string, CheckOption> checks = {
        { "UCI_Chess960", { castling::is_frc, [](bool b) { castling::is_frc = b; } } },
        { "Ponder", { allow_ponder, [](bool b) { allow_ponder = b; } } }
    };
    std::unordered_map<std::string, StringOption> strings = {
        { "SyzygyPath", { "None", { [](std::string s) { syzygy_path_handler(s); } } } }
    };
}
