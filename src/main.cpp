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

#include "uci.h"
#include "test.h"
#include "lookups.h"
#include "evaluate.h"

#include "lc0nn.h"

lczero::NetworkFactory* network_factory;
lczero::Weights weights;
std::unique_ptr<lczero::Network> network;

int main()
{
    std::ios_base::sync_with_stdio(false);
    std::cout.setf(std::ios::unitbuf);
    lookups::init();
    eval::init();

    std::string nn_backend = "cudnn";
    network_factory = lczero::NetworkFactory::Get();
    weights = lczero::LoadWeightsFromFile("weights_522.txt.gz");
    network = network_factory->Create(nn_backend, weights, lczero::OptionsDict {});
    std::cout << nn_backend << " ready!\n";

#ifdef TEST
    test();
    return 0;
#endif

    std::string word;
    while (true) {
        std::cin >> word;
        if (word == "uci")
        {
            uci::init();
            break;
        }
        else
        {
            std::cout << "Unrecognized protocol!" << std::endl;
        }
    }
    return 0;
}
