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

#ifndef MCTS_H
#define MCTS_H

#include "utils.h"
#include "position.h"

#include "lc0nn.h"

class Node
{
public:
    Node(Position& _pos, float _prior, float _payoff, Move move)
        :_pos(_pos), _move(move), _prior(_prior), _payoff(_payoff), _visits(0) {}
    Node(Position& _pos, float _prior, Move move) : Node(_pos, _prior, 0.0f, move) {}

    // Getters
    const Position& pos() const { return _pos; }
    float prior() const { return _prior; }
    float payoff() const { return _payoff; }
    int visits() const { return _visits; }
    bool expanded() const { return !_children.empty(); }
    bool flipped() const { return _pos.is_flipped(); }
    void display() const { _pos.display(); }
    Move move() const { return _move; }
    std::vector<Node>& children() { return _children; }

    // Modifiers
    void inc_visits() { ++_visits; }

    // Core functions
    Node* most_visited_child();
    float ucb(float part_ucb) const;
    void update_payoff(float child_qval);
    void compute(std::vector<Node*>& parents, const PositionHistory& pos_hist);

private:
    Position _pos;
    Move _move;
    float _prior;
    float _payoff;
    int _visits;
    std::vector<Node> _children;
};

class GameTree
{
public:
    GameTree(Position& pos, PositionHistory& pos_hist) : root(pos, 0.0f, 0), pos_hist(pos_hist) {}
    std::pair<Node*, std::vector<Node*>> select();
    std::vector<Move> pv();
    void search();

private:
    Node root;
    PositionHistory pos_hist;
};

#endif
