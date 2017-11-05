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
#include "time_manager.h"

#define INITIAL_POSITION ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")

TimeManager time_manager;

Move get_parsed_move(Position& pos, std::string& move_str)
{
    std::vector<Move> mlist;
    pos.generate_movelist(mlist);
    for (Move move : mlist) {
        if (get_move_string(move, pos.is_flipped()) == move_str)
            return move;
    }
    std::cout << "CANNOT PARSE MOVE " << move_str << "!" << std::endl;
    return 0;
}

std::string get_pv_string(std::vector<Move>& pv, bool flipped)
{
    std::string pv_string = "";
    for (int i = 0; i < pv.size(); ++i, flipped = !flipped) {
        pv_string += get_move_string(pv[i], flipped);
        if (i != pv.size() - 1)
            pv_string += " ";
    }
    return pv_string;
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
        i32 depth;
        if (!(stream >> depth))
            depth = 1;

        u64 count = u64(1);
        for (i32 d = 1; d <= depth; ++d) {
            time_ms t1 = utils::curr_time();
            count = pos.perft(d);
            time_ms t2 = utils::curr_time();
            std::cout << "info depth " << d
                      << " time " << (t2 - t1)
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
        std::string word;
        time_manager.time_dependent = false;
        time_manager.start_time = utils::curr_time();
        time_manager.end_time = time_manager.start_time;
        while (stream >> word) {
            if (word == "infinite")
            {
                time_manager.time_dependent = false;
            }
            else if (word == "movetime")
            {
                time_manager.time_dependent = true;
                time_ms movetime;
                stream >> movetime;
                time_manager.end_time += movetime;
            }
        }
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

    void print_currmove(Move move, i32 move_num, time_ms start_time, bool flipped)
    {
        time_ms curr_time = utils::curr_time();
        time_ms time_passed = curr_time - start_time;
        if (time_passed >= 1000)
        {
            std::cout << "info"
                      << " currmovenumber " << move_num
                      << " currmove " << get_move_string(move, flipped)
                      << " time " << time_passed
                      << std::endl;
        }
    }

    void print_search(int score, int depth, u64 nodes, time_ms time,
                      std::vector<Move>& pv, bool flipped)
    {
        std::cout << "info"
                  << " score cp " << score
                  << " depth " << depth
                  << " nodes " << nodes
                  << " time " << time
                  << " pv " << get_pv_string(pv, flipped)
                  << std::endl;
    }
}
