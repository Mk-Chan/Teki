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

// This file contains the PositionHistory class which is a dependency of the
// Leela Chess neural framework.

#ifndef POSITION_HISTORY_H
#define POSITION_HISTORY_H

#include "../position.h"

enum class GameResult { UNDECIDED, WHITE_WON, DRAW, BLACK_WON };

class PositionHistory
{
public:
    PositionHistory() = default;
    PositionHistory(const PositionHistory& other) = default;

    // Returns first position of the game (or fen from which it was initialized).
    const Position& Starting() const { return positions_.front(); }

    // Returns the latest position of the game.
    const Position& Last() const { return positions_.back(); }

    // N-th position of the game, 0-based.
    const Position& GetPositionAt(int idx) const { return positions_[idx]; }

    // Trims position to a given size.
    void Trim(int size) {
        positions_.erase(positions_.begin() + size, positions_.end());
    }

    void TrimTo8() {
        int size = positions_.size();
        if (size <= 8)
            return;
        positions_.erase(positions_.begin(), positions_.begin() + size - 8);
    }

    // Number of positions in history.
    int GetLength() const { return positions_.size(); }

    // Resets the position to a given state.
    void Reset(const Position& board, int no_capture_ply=0, int game_ply=0);

    // Appends a position to history.
    void Append(Move m);

    // Pops last move from history.
    void Pop() { positions_.pop_back(); }

    // Finds the endgame state (win/lose/draw/nothing) for the last position.
    GameResult ComputeGameResult() const;

    // Returns whether next move is history should be black's.
    bool IsBlackToMove() const { return Last().is_flipped(); }

    // Builds a hash from last X positions.
    uint64_t HashLast(int positions) const;

private:
    int ComputeLastMoveRepetitions() const;
    std::vector<Position> positions_;
};

inline void PositionHistory::Reset(const Position& board, int no_capture_ply, int game_ply)
{
  positions_.clear();
  positions_.emplace_back(board);
}

inline void PositionHistory::Append(Move m) {
    Position p {positions_.back()};
    p.make_move(m);
    positions_.push_back(p);
    TrimTo8();
}

#endif
