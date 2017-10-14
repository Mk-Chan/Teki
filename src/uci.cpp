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
#include "move.h"

#define INITIAL_POSITION ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")

Move get_parsed_move(Position& pos, std::string& move_str)
{
    std::vector<Move> mlist = pos.get_movelist();
    for (Move move : mlist) {
        if (get_move_string(move, pos.is_flipped()) == move_str)
            return move;
    }
    std::cout << "CANNOT PARSE MOVE " << move_str << "!" << std::endl;
    return 0;
}

namespace handler
{
    void uci()
    {
        std::cout << "id name " << NAME << '\n'
                  << "id author " << AUTHOR << std::endl;
        std::cout << "uciok" << std::endl;
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

        u64 count;
        for (u32 d = 1; d <= depth; ++d) {
            count = pos.perft(d);
            std::cout << "info depth " << d
                      << " nodes " << count
                      << std::endl;
        }
        std::cout << "nodes " << count << std::endl;
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
            std::string move_str;
            while (stream >> move_str)
                pos.make_move(get_parsed_move(pos, move_str));
        }
    }

    void go(Position& pos, std::stringstream& stream)
    {
        Move move = pos.best_move();
        std::cout << "bestmove " << get_move_string(move, pos.is_flipped())
                  << std::endl;
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
        else if (word == "go") handler::go(pos, stream);
        else if (word == "quit") break;
    }
}

namespace uci
{
    void init()
    {
        handler::uci();
        loop();
    }
}
