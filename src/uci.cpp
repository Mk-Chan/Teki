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

#include <iostream>
#include <string>
#include <sstream>
#include <thread>

#include "uci.h"
#include "mcts.h"
#include "options.h"
#include "position.h"
#include "controller.h"

#include "lc0nn.h"

#define INITIAL_POSITION ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")

volatile bool searching = false;

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

namespace handler
{
    void uci()
    {
        std::cout << "id name " << NAME << '\n'
                  << "id author " << AUTHOR << std::endl;
        for (auto& [name, option] : options::checks) {
            std::string val = option.default_value ? "true" : "false";
            std::cout << "option name " << name << " type check"
                      << " default " << val
                      << std::endl;
        }
        for (auto& [name, option] : options::spins) {
            std::cout << "option name " << name << " type spin"
                      << " default " << option.value
                      << " min " << option.min
                      << " max " << option.max
                      << std::endl;
        }
        for (auto& [name, option]: options::strings) {
            std::cout << "option name " << name << " type string"
                      << " default " << option.value
                      << std::endl;
        }
        std::cout << "uciok" << std::endl;
    }

    void d(Position& pos)
    {
        pos.display();
    }

    void ucinewgame()
    {
        ;
    }

    void isready()
    {
        std::cout << "readyok" << std::endl;
    }

    void stop()
    {
        controller.analyzing = false;
        controller.stop_search = true;
        while (searching)
            continue;
    }

    void perft(Position& pos, std::stringstream& stream)
    {
        int depth;
        if (!(stream >> depth))
            depth = 1;

        u64 count = u64(1);
        for (int d = 1; d <= depth; ++d) {
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

    void setoption(std::stringstream& stream)
    {
        std::string word;
        stream >> word;
        if (word != "name")
            return;

        stream >> word;
        if (options::spins.find(word) != options::spins.end())
        {
            std::string name = word;

            stream >> word;
            if (word != "value")
                return;

            int value;
            stream >> value;

            options::spins[name].setoption(value);
        }
        else if (options::checks.find(word) != options::checks.end())
        {
            std::string name = word;

            stream >> word;
            if (word != "value")
                return;

            std::string value_str;
            stream >> value_str;

            bool value;
            if (value_str == "true")
                value = true;
            else if (value_str == "false")
                value = false;
            else
                return;

            options::checks[name].setoption(value);
        }
        else if (options::strings.find(word) != options::strings.end())
        {
            std::string name = word;

            stream >> word;
            if (word != "value")
                return;

            std::string value;
            stream >> value;

            options::strings[name].setoption(value);
        }
    }

    void position(Position& pos, std::stringstream& stream, PositionHistory& pos_hist)
    {
        handler::stop();

        std::string word;
        stream >> word;
        if (word == "startpos")
        {
            std::stringstream stream {INITIAL_POSITION};
            pos.init(stream);
            pos_hist.Reset(pos);
        }
        else if (word == "fen")
        {
            pos.init(stream);
            pos_hist.Reset(pos);
        }

        if (stream >> word && word == "moves")
        {
            std::string move_str;
            while (stream >> move_str) {
                pos.make_move(get_parsed_move(pos, move_str));
                pos_hist.Append(pos);
            }
        }
    }

    void go(Position& pos, std::stringstream& stream, PositionHistory& pos_hist)
    {
        std::string word;
        controller.stop_search = false;
        controller.time_dependent = false;
        controller.limited_search = false;
        controller.analyzing = false;
        controller.max_ply = MAX_PLY;
        controller.start_time = utils::curr_time();
        controller.end_time = controller.start_time;
        controller.max_nodes = ~(0ULL);
        time_ms time_to_go = 1000;
        if (pos_hist.GetLength() == 0)
            pos_hist.Reset(pos);
        int moves_to_go = 35,
            increment = 0;
        while (stream >> word) {
            if (word == "infinite" || word == "ponder")
            {
                controller.time_dependent = false;
                controller.analyzing = true;
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
            else if (word == "searchmoves")
            {
                controller.limited_search = true;
                controller.search_moves.clear();
                std::string move_str;
                while (stream >> move_str) {
                    Move move = get_parsed_move(pos, move_str);
                    controller.search_moves.push_back(move);
                }
            }
            else if (word == "movestogo")
            {
                stream >> moves_to_go;
            }
            else if (word == "nodes")
            {
                controller.time_dependent = false;
                std::uint64_t max_nodes;
                if (stream >> max_nodes)
                    controller.max_nodes = max_nodes;
            }
            else if (word == "depth")
            {
                controller.time_dependent = false;
                int limited_depth;
                if (stream >> limited_depth)
                    controller.max_ply = limited_depth;
            }
        }

        if (controller.time_dependent)
        {
            controller.end_time +=
                (time_to_go + (moves_to_go - 1) * increment) / moves_to_go;
            if (moves_to_go == 1)
                controller.end_time -= 50;
        }

        if (!searching)
        {
            searching = true;
            std::thread search_thread([&pos, &pos_hist]() {
                GameTree gt {pos, pos_hist};
                gt.search();
                searching = false;
            });
            search_thread.detach();
        }
    }

    void ponderhit()
    {
        controller.time_dependent = true;
        controller.analyzing = false;
    }
}

void loop()
{
    Position pos;
    PositionHistory pos_hist;
    std::stringstream stream {INITIAL_POSITION};
    pos.init(stream);
    std::string line, word;
    while (true)
    {
        std::getline(std::cin, line);
        std::stringstream stream {line};
            
        stream >> word;
        if (word == "d") handler::d(pos);
        else if (word == "ucinewgame") handler::ucinewgame();
        else if (word == "setoption") handler::setoption(stream);
        else if (word == "isready") handler::isready();
        else if (word == "perft") handler::perft(pos, stream);
        else if (word == "position") handler::position(pos, stream, pos_hist);
        else if (word == "go") handler::go(pos, stream, pos_hist);
        else if (word == "ponderhit") handler::ponderhit();
        else if (word == "stop") handler::stop();
        else if (word == "quit") break;
    }
    handler::stop();
}

namespace uci
{
    void init()
    {
        handler::uci();
        loop();
    }

    void print_currmove(Move move, int move_num, time_ms start_time, bool flipped)
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

    void print_search(int score, int depth, int bound, time_ms time,
                      std::vector<Move>& pv, bool flipped)
    {
        std::cout << "info";
        std::cout << " score ";
        if (std::abs(score) < MAX_MATE_VALUE)
        {
            std::cout << "cp " << score;
        }
        else
        {
            std::cout << "mate ";
            if (score < 0)
                std::cout << (-score - MATE) / 2;
            else
                std::cout << (-score + MATE + 1) / 2;
        }
        if (bound == UPPER_BOUND)
            std::cout << " upperbound";
        else if (bound == LOWER_BOUND)
            std::cout << " lowerbound";
        std::cout << " depth " << depth;
        std::cout << " nodes " << controller.nodes_searched;
        std::cout << " time " << time;
        if (time > 1000)
            std::cout << " nps " << (controller.nodes_searched * 1000) / time;
        std::cout << " pv " << get_pv_string(pv, flipped);
        std::cout << std::endl;
    }

    std::string get_pv_string(std::vector<Move>& pv, bool flipped)
    {
        std::string pv_string = "";
        for (unsigned i = 0; i < pv.size(); ++i, flipped = !flipped) {
            pv_string += get_move_string(pv[i], flipped);
            if (i != pv.size() - 1)
                pv_string += " ";
        }
        return pv_string;
    }
}
