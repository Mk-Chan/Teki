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

#include "tt.h"
#include "uci.h"
#include "position.h"
#include "controller.h"

#define INITIAL_POSITION ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")

TranspositionTable tt;
Controller controller;

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
        tt.clear();
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
        controller.stop_search = false;
        controller.time_dependent = false;
        controller.start_time = utils::curr_time();
        controller.end_time = controller.start_time;
        time_ms time_to_go = 1000;
        int moves_to_go = 35,
            increment = 0;
        while (stream >> word) {
            if (word == "infinite")
            {
                controller.time_dependent = false;
            }
            else if (word == "movetime")
            {
                controller.time_dependent = true;
                time_ms movetime;
                stream >> movetime;
                time_to_go = movetime;
                moves_to_go = 1;
            }
            else if (word == "wtime")
            {
                controller.time_dependent = true;
                time_ms wtime;
                stream >> wtime;
                if (!pos.is_flipped())
                    time_to_go = wtime;
            }
            else if (word == "btime")
            {
                controller.time_dependent = true;
                time_ms btime;
                stream >> btime;
                if (pos.is_flipped())
                    time_to_go = btime;
            }
            else if (word == "winc")
            {
                controller.time_dependent = true;
                time_ms winc;
                stream >> winc;
                if (!pos.is_flipped())
                    increment = winc;
            }
            else if (word == "binc")
            {
                controller.time_dependent = true;
                time_ms binc;
                stream >> binc;
                if (pos.is_flipped())
                    increment = binc;
            }
            else if (word == "movestogo")
            {
                stream >> moves_to_go;
            }
        }

        if (controller.time_dependent)
        {
            controller.end_time +=
                (time_to_go + (moves_to_go - 1) * increment) / moves_to_go;
            if (moves_to_go == 1)
                controller.end_time -= 50;
        }

        std::thread search_thread([&pos]() {
            Move move = pos.best_move();
            std::cout << "bestmove " << get_move_string(move, pos.is_flipped())
                      << std::endl;
        });
        search_thread.detach();
    }

    void stop()
    {
        controller.stop_search = true;
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
        else if (word == "stop") handler::stop();
        else if (word == "quit") break;
    }
    controller.stop_search = true;
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
                      << " nodes " << controller.nodes_searched
                      << " time " << time_passed
                      << std::endl;
        }
    }

    void print_search(int score, int depth, time_ms time, std::vector<Move>& pv,
                      bool flipped)
    {
        std::cout << "info"
                  << " score cp " << score
                  << " depth " << depth
                  << " nodes " << controller.nodes_searched
                  << " time " << time
                  << " pv " << get_pv_string(pv, flipped)
                  << std::endl;
    }
}
