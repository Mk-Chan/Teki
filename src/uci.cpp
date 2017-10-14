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
#include "position.h"

#define INITIAL_POSITION ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")

namespace uci
{
    namespace handler
    {
        void uci()
        {
            std::cout << "id name " << NAME << '\n'
                      << "id author " << AUTHOR << std::endl;
        }

        void d(Position& pos)
        {
            pos.display();
        }

        void ucinewgame()
        {
            // Reset search/eval state (hashtables, history etc)
        }

        void isready()
        {
            std::cout << "readyok" << std::endl;
        }

        void perft(Position& pos, std::stringstream& stream)
        {
            u32 depth;
            if (!(stream >> depth))
                depth = 1;
            u64 count = pos.perft(depth);
            std::cout << count << std::endl;
        }

        void position(Position& pos, std::stringstream& stream)
        {
            std::string word;
            stream >> word;
            if (word == "startpos")
            {
                std::stringstream stream = std::stringstream(INITIAL_POSITION);
                pos.init(stream);
            }
            else if (word == "fen")
            {
                pos.init(stream);
            }

            if (stream >> word && word == "moves")
            {
                // Do moves here
            }
        }
    }

    void loop()
    {
        Position pos;
        std::stringstream stream = std::stringstream(INITIAL_POSITION);
        pos.init(stream);
        std::string line, word;
        while (true)
        {
            std::getline(std::cin, line);
            std::stringstream stream(line);

            stream >> word;
            if (word == "d") handler::d(pos);
            else if (word == "ucinewgame") handler::ucinewgame();
            else if (word == "isready") handler::isready();
            else if (word == "perft") handler::perft(pos, stream);
            else if (word == "position") handler::position(pos, stream);
            else if (word == "quit") break;
        }
    }

    void init()
    {
        handler::uci();
        loop();
    }
}
