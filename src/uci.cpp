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

#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include "uci.h"

namespace uci
{
    namespace handler
    {
        void uci()
        {
            std::cout << "id name " << NAME << '\n'
                      << "id author " << AUTHOR << std::endl;
        }

        void ucinewgame()
        {
            // Reset search/eval state (hashtables, history etc)
        }

        void isready()
        {
            std::cout << "readyok" << std::endl;
        }

        void perft()
        {
            
        }
    }

    void loop()
    {
        std::string line, word;
        while (true)
        {
            std::getline(std::cin, line);
            std::stringstream stream(line);

            stream >> word;
            if (word == "uci") handler::uci();
            else if (word == "ucinewgame") handler::ucinewgame();
            else if (word == "isready") handler::isready();
            else if (word == "perft") handler::perft();
            else if (word == "quit") break;
        }
    }

    void init()
    {
        std::ios_base::sync_with_stdio(false);
        std::cout.setf(std::ios::unitbuf);
        loop();
    }
}
