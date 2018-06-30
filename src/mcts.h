/*
MIT License

Copyright (c) 2018 Manik Charan

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
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
