#include "board.h"
#include "common.h"
#include "eval_normal.h"
#include "movegen.h"
#include "movegen_normal.h"
#include "piece.h"
#include "stopwatch.h"
#include "u64_op.h"

namespace eval {

namespace {
// Piece values.
namespace pv {
const int KING = 0;
const int QUEEN = 9;
const int ROOK = 5;
const int BISHOP = 3;
const int KNIGHT = 3;
const int PAWN = 1;
}  // namespace pv

const int MATERIAL_FACTOR = 25;

// These values are basically crap.
const int OPENING_PAWNS_STRENGTH_FACTOR = 4;
const int MIDDLE_PAWNS_STRENGTH_FACTOR = 2;

const int sq_strength[] = {
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 2, 2, 2, 2, 2, 2, 1,
  1, 2, 3, 3, 3, 3, 2, 1,
  1, 2, 3, 5, 5, 3, 2, 1,
  1, 2, 3, 5, 5, 3, 2, 1,
  1, 2, 3, 3, 3, 3, 3, 1,
  1, 2, 2, 2, 2, 2, 2, 1,
  1, 1, 1, 1, 1, 1, 1, 1
};

}  // namespace

int EvalNormal::PieceValDifference() const {
  const int white_val =
      PopCount(board_->BitBoard(KING))    * pv::KING   +
      PopCount(board_->BitBoard(QUEEN))   * pv::QUEEN  +
      PopCount(board_->BitBoard(PAWN))    * pv::PAWN   +
      PopCount(board_->BitBoard(BISHOP))  * pv::BISHOP +
      PopCount(board_->BitBoard(KNIGHT))  * pv::KNIGHT +
      PopCount(board_->BitBoard(ROOK))    * pv::ROOK;
  const int black_val =
      PopCount(board_->BitBoard(-KING))   * pv::KING   +
      PopCount(board_->BitBoard(-QUEEN))  * pv::QUEEN  +
      PopCount(board_->BitBoard(-PAWN))   * pv::PAWN   +
      PopCount(board_->BitBoard(-BISHOP)) * pv::BISHOP +
      PopCount(board_->BitBoard(-KNIGHT)) * pv::KNIGHT +
      PopCount(board_->BitBoard(-ROOK))   * pv::ROOK;

  return (board_->SideToMove() == Side::WHITE) ? (white_val - black_val)
                                               : (black_val - white_val);
}

int EvalNormal::Evaluate() {
  const Side side = board_->SideToMove();
  MoveArray move_array;
  movegen_->GenerateMoves(&move_array);
  if (move_array.size() == 0) {
    const U64 attack_map =
        movegen::ComputeAttackMap(*board_, OppositeSide(side));
    if (attack_map & board_->BitBoard(PieceOfSide(KING, side))) {
      return -WIN;
    } else {
      return DRAW;  // stalemate
    }
  }
  int score = MATERIAL_FACTOR * PieceValDifference();
  for (int i = 0; i < move_array.size(); ++i) {
    const Move& move = move_array.get(i);
    board_->MakeMove(move);
    U64 attack_map = movegen::ComputeAttackMap(*board_, side);
    if (attack_map &
        board_->BitBoard(PieceOfSide(KING, OppositeSide(side)))) {
      MoveArray opp_move_array;
      movegen_->GenerateMoves(&opp_move_array);
      if (opp_move_array.size() == 0) {
        score = WIN;
      }
    }
    board_->UnmakeLastMove();
  }
  if (score == WIN) {
    return score;
  }
  U64 self_pawns = board_->BitBoard(PieceOfSide(PAWN, side));
  U64 opp_pawns = board_->BitBoard(PieceOfSide(PAWN, OppositeSide(side)));
  U64Op op1(self_pawns), op2(opp_pawns);
  int index, self_pawns_strength = 0, opp_pawns_strength = 0;
  while ((index = op1.NextRightMostBitIndex()) != -1) {
    self_pawns_strength += sq_strength[index];
  }
  while ((index = op2.NextRightMostBitIndex()) != -1) {
    opp_pawns_strength += sq_strength[index];
  }

  const int pawns_strength =
      (self_pawns_strength - opp_pawns_strength) *
      ((board_->Ply() <= 10) ? OPENING_PAWNS_STRENGTH_FACTOR
                             : MIDDLE_PAWNS_STRENGTH_FACTOR);

  if (board_->Ply() > 8) {
    const int main_row = ((side == Side::WHITE) ? 0 : 7);
    // Better to move the bishop.
    if ((board_->BitBoard(PieceOfSide(BISHOP, side)) &
         ((1ULL << INDX(main_row, 2)) | (1ULL << INDX(main_row, 5))))) {
      score -= 5;
    }

    // Better to move the knight.
    if ((board_->BitBoard(PieceOfSide(KNIGHT, side)) &
         ((1ULL << INDX(main_row, 1)) | (1ULL << INDX(main_row, 6))))) {
      score -= 5;
    }
  }

  return score + pawns_strength + (move_array.size() / 15);
}

int EvalNormal::Result() const {
  const Side side = board_->SideToMove();
  MoveArray move_array;
  movegen_->GenerateMoves(&move_array);
  if (move_array.size() == 0) {
    const U64 attack_map =
        movegen::ComputeAttackMap(*board_, OppositeSide(side));
    if (attack_map & board_->BitBoard(PieceOfSide(KING, side))) {
      return -WIN;
    } else {
      return DRAW;  // stalemate
    }
  }
  return -1;
}

}  // namespace eval
