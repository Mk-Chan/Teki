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
#include "neural/loader.h"
#include "neural/factory.h"
#include "neural/encoder.h"
#include "neural/optionsdict.h"
#include "neural/move_index.h"

void test_nn()
{
    auto network_factory = lczero::NetworkFactory::Get();
    auto weights =  lczero::LoadWeightsFromFile("weights_479.txt.gz");
    auto network = network_factory->Create("cudnn", weights, lczero::OptionsDict {});
    std::cout << "Enter FEN\n";

    std::string line;
    std::getline(std::cin, line);

    Position pos;
    std::stringstream ss {line};
    pos.init(ss);

    std::vector<Move> mlist;

    while (true) {
        //pos.display();
        //std::cout << std::endl;

        mlist.clear();
        pos.generate_legal_movelist(mlist);

        if (mlist.empty() || pos.is_drawn())
            break;

        auto comp = network->NewComputation();
        for (Move& m : mlist) {
            auto pos_hist = PositionHistory {};

            Position p {pos};
            pos_hist.Reset(p);

            pos_hist.Append(m);
            p = pos_hist.Last();

            auto plane = lczero::EncodePositionForNN(pos_hist, pos_hist.GetLength());
            lczero::InputPlanes planes {plane};
            comp->AddInput(std::move(planes));
        }

        comp->ComputeBlocking();

        Move best_move = 0;
        float best_p = -100;
        for (Move& m : mlist) {
            Position np {pos};
            np.make_move(m);
            //np.display();
            auto st = get_move_string(m, false);
            int i = -1;
            for (i = 0; i < 1858; ++i)
                if (st == kIdxToMove[i])
                    break;
            //std::cout << "i: " << i << ", move: " << kIdxToMove[i] << std::endl;
            float q = comp->GetQVal(0);
            float p = comp->GetPVal(0, i);
            if (p > best_p)
            {
                best_move = m;
                best_p = p;
            }
            //std::cout << "Q: " << q << ", P: " << p << std::endl << std::endl;
        }

        std::cout << get_move_string(best_move, pos.is_flipped()) << " ";
        pos.make_move(best_move);
    }
    std::cout << std::endl << "Done!\n";
}

void test()
{
    test_nn();
}
