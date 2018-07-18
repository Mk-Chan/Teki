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

enum Policy
{
    SELECTION, VALUE
};

class Node
{
public:
    Node(Position& pos) : pos(pos), visits(0), wins(0), move(0), self_p(0), q_val(0) {}
    void set_move(Move move) { this->move = move; }
    Move get_move() { return move; }
    std::vector<Node>& get_children() { return children; }
    int get_visits() { return visits; }
    int get_wins() { return wins; }
    void inc_visits() { ++visits; }
    void inc_wins() { ++wins; }
    void display() { pos.display(); }
    Position& get_position() { return pos; }
    void generate_moves() { pos.generate_legal_movelist(mlist); }
    std::vector<Move> get_mlist() { return mlist; }
    Node* latest_child() { return &children.back(); }
    bool flipped() { return pos.is_flipped(); }
    float get_q() { return q_val; }
    float get_p() { return self_p; }
    void set_q(float q) { q_val = q; }
    void push_p(float p) { children_p.push_back(p); }
    void set_p(float p) { self_p = p; }

    bool is_terminal();
    bool expanded();
    bool has_moves();
    Node* expand();

    template <Policy policy>
    double score(int parent_visits=0);
    template <Policy policy>
    Node* get_child();

    void compute(std::vector<Node*>& parents);

private:
    Move next_move();
    float next_p();

    Position pos;
    std::vector<Move> mlist;
    std::vector<float> children_p;
    std::vector<Node> children;
    int visits;
    int wins;
    Move move;
    float self_p;
    float q_val;
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
