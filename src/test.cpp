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

#include "move.h"
#include "lc0nn.h"

int lc0_move_index(std::string move_str)
{
    int i = -1;
    for (i = 0; i < 1858; ++i)
        if (move_str == kIdxToMove[i])
            break;
    return i;
}

void test_nn()
{
    std::cout << "Enter FEN\n";

    std::string line;
    std::getline(std::cin, line);

    Position pos;
    std::stringstream ss {line};
    pos.init(ss);

    std::vector<Move> mlist;

    while (true) {
        pos.display();
        std::cout << std::endl;

        mlist.clear();
        pos.generate_legal_movelist(mlist);

        if (mlist.empty() || pos.is_drawn())
            break;

        auto comp = network->NewComputation();
        auto pos_hist = PositionHistory {};
        pos_hist.Reset(pos);
        pos_hist.Last().display();
        auto planes = lczero::EncodePositionForNN(pos_hist, 8);
        comp->AddInput(std::move(planes));
        comp->ComputeBlocking();

        float q = comp->GetQVal(0);
        std::cout << "q: " << q << std::endl;

        Move best_move = 0;
        float best_p = -100;
        for (Move& m : mlist) {
            std::string move_str = get_move_string(m, false);
            int i = lc0_move_index(move_str);
            float p = comp->GetPVal(0, i);
            if (p > best_p)
            {
                best_move = m;
                best_p = p;
            }
            std::cout << move_str << ": p = " << p << std::endl;
        }

        std::cout << get_move_string(best_move, pos.is_flipped());
        break;
        pos.make_move(best_move);
    }
    std::cout << std::endl << "Done!\n";
}

void test()
{
    test_nn();
}
