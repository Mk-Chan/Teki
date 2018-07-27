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

#ifndef LC0NN_H
#define LC0NN_H

#include <numeric>

#include "neural/loader.h"
#include "neural/factory.h"
#include "neural/encoder.h"
#include "neural/optionsdict.h"
#include "neural/move_index.h"

extern lczero::NetworkFactory* network_factory;
extern lczero::Weights weights;
extern std::unique_ptr<lczero::Network> network;

inline int lc0_move_index(std::string move_str)
{
    int i = -1;
    for (i = 0; i < 1858; ++i)
        if (move_str == kIdxToMove[i])
            break;
    return i;
}

inline std::vector<float> softmax(std::vector<float> pvals)
{
    float sum = std::accumulate(pvals.begin(), pvals.end(), 0.0f);
    std::vector<float> probs;
    for (float p : pvals) {
        probs.push_back(p / sum);
    }
    return probs;
}

#endif
