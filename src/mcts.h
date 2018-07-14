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

enum ResultType
{
    PENDING, DRAW, WIN, LOSS
};

enum Policy
{
    SELECTION, VALUE
};

class Node
{
public:
    Node(Position& pos) : pos(pos), simulations(0), wins(0), move(0) { this->generate_moves();}
    void set_move(Move move) { this->move = move; }
    Move get_move() { return move; }
    std::vector<Node>& get_children() { return children; }
    u64 get_simulations() { return simulations; }
    u64 get_wins() { return wins; }
    void inc_simulations() { ++simulations; }
    void inc_wins() { ++wins; }
    void display() { pos.display(); }
    Position& get_position() { return pos; }
    void generate_moves() { pos.generate_legal_movelist(mlist); }
    std::vector<Move> get_mlist() { return mlist; }
    Node* latest_child() { return &children.back(); }
    bool flipped() { return pos.is_flipped(); }

    bool is_terminal();
    bool expanded();
    bool has_moves();
    Node* expand();

    template <Policy policy>
    double score(u64 parent_simulations=0);
    template <Policy policy>
    Node* get_child();

    int simulate();

private:
    Move next_move();

    Position pos;
    std::vector<Move> mlist;
    std::vector<Node> children;
    u64 simulations;
    u64 wins;
    Move move;
};

class GameTree
{
public:
    GameTree(Position& pos) : root(pos) {}
    std::pair<Node*, std::vector<Node*>> select();
    std::vector<Move> pv();
    void search();

private:
    Node root;
};

#endif
