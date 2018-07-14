/*
  This file is part of Leela Chess Zero.
  Copyright (C) 2018 The LCZero Authors

  Leela Chess is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Leela Chess is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Leela Chess.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "encoder.h"
#include <algorithm>

namespace lczero {

namespace {
const int kMoveHistory = 8;
const int kPlanesPerBoard = 13;
const int kAuxPlaneBase = kPlanesPerBoard * kMoveHistory;
}  // namespace

InputPlanes EncodePositionForNN(const PositionHistory& history,
                                int history_planes) {
  InputPlanes result(kAuxPlaneBase + 8);

  {
    const Position& board = history.Last();
    const bool we_are_black = board.is_flipped();
    if (board.get_castling_rights() & US_OOO) result[kAuxPlaneBase + 0].SetAll();
    if (board.get_castling_rights() & US_OO) result[kAuxPlaneBase + 0].SetAll();
    if (board.get_castling_rights() & THEM_OOO) result[kAuxPlaneBase + 0].SetAll();
    if (board.get_castling_rights() & THEM_OO) result[kAuxPlaneBase + 0].SetAll();
    if (we_are_black) result[kAuxPlaneBase + 4].SetAll();
    result[kAuxPlaneBase + 5].Fill(history.Last().get_half_moves());
    // Plane kAuxPlaneBase + 6 used to be movecount plane, now it's all zeros.
    // Plane kAuxPlaneBase + 7 is all ones to help NN find board edges.
    result[kAuxPlaneBase + 7].SetAll();
  }

  int history_idx = history.GetLength() - 1;
  for (int i = 0; i < std::min(history_planes, kMoveHistory);
       ++i, --history_idx) {
    if (history_idx < 0) break;
    const Position& position = history.GetPositionAt(history_idx);
    const Position& board = position;

    const int base = i * kPlanesPerBoard;
    result[base + 0].mask = board.piece_bb(PAWN, US);
    result[base + 1].mask = board.piece_bb(KNIGHT, US);
    result[base + 2].mask = board.piece_bb(BISHOP, US);
    result[base + 3].mask = board.piece_bb(ROOK, US);
    result[base + 4].mask = board.piece_bb(QUEEN, US);
    result[base + 5].mask = board.piece_bb(KING, US);

    result[base + 6].mask = board.piece_bb(PAWN, THEM);
    result[base + 7].mask = board.piece_bb(KNIGHT, THEM);
    result[base + 8].mask = board.piece_bb(BISHOP, THEM);
    result[base + 9].mask = board.piece_bb(ROOK, THEM);
    result[base + 10].mask = board.piece_bb(QUEEN, THEM);
    result[base + 11].mask = board.piece_bb(KING, THEM);

    if (position.is_repetition()) result[base + 12].SetAll();
  }

  return result;
}

}  // namespace lczero
