/*
  Weiss is a UCI compliant chess engine.
  Copyright (C) 2020  Terje Kirstihagen

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "bitboard.h"
#include "board.h"
#include "move.h"
#include "psqt.h"


#define HASH_PCE(piece, sq) (pos->key ^= PieceKeys[(piece)][(sq)])
#define HASH_CA             (pos->key ^= CastleKeys[pos->castlingRights])
#define HASH_SIDE           (pos->key ^= SideKey)
#define HASH_EP             (pos->key ^= PieceKeys[EMPTY][pos->epSquare])


static const uint8_t CastlePerm[64] = {
    13, 15, 15, 15, 12, 15, 15, 14,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
     7, 15, 15, 15,  3, 15, 15, 11
};


// Remove a piece from a square sq
static void ClearPiece(Position *pos, const Square sq, const bool hash) {

    const Piece piece = pieceOn(sq);
    const Color color = ColorOf(piece);
    const PieceType pt = PieceTypeOf(piece);

    assert(ValidPiece(piece));

    // Hash out the piece
    if (hash)
        HASH_PCE(piece, sq);

    // Set square to empty
    pieceOn(sq) = EMPTY;

    // Update material
    pos->material -= PSQT[piece][sq];

    // Update phase
    pos->basePhase -= PhaseValue[piece];
    pos->phase = (pos->basePhase * 256 + 12) / 24;

    // Update non-pawn count
    pos->nonPawnCount[color] -= NonPawn[piece];

    // Update bitboards
    pieceBB(ALL)   ^= SquareBB[sq];
    pieceBB(pt)    ^= SquareBB[sq];
    colorBB(color) ^= SquareBB[sq];
}

// Add a piece piece to a square
static void AddPiece(Position *pos, const Square sq, const Piece piece, const bool hash) {

    const Color color = ColorOf(piece);
    const PieceType pt = PieceTypeOf(piece);

    // Hash in piece at square
    if (hash)
        HASH_PCE(piece, sq);

    // Update square
    pieceOn(sq) = piece;

    // Update material
    pos->material += PSQT[piece][sq];

    // Update phase
    pos->basePhase += PhaseValue[piece];
    pos->phase = (pos->basePhase * 256 + 12) / 24;

    // Update non-pawn count
    pos->nonPawnCount[color] += NonPawn[piece];

    // Update bitboards
    pieceBB(ALL)   |= SquareBB[sq];
    pieceBB(pt)    |= SquareBB[sq];
    colorBB(color) |= SquareBB[sq];
}

// Move a piece from one square to another
static void MovePiece(Position *pos, const Square from, const Square to, const bool hash) {

    const Piece piece = pieceOn(from);
    const Color color = ColorOf(piece);
    const PieceType pt = PieceTypeOf(piece);

    assert(ValidPiece(piece));
    assert(pieceOn(to) == EMPTY);

    // Hash out piece on old square, in on new square
    if (hash)
        HASH_PCE(piece, from),
        HASH_PCE(piece, to);

    // Set old square to empty, new to piece
    pieceOn(from) = EMPTY;
    pieceOn(to)   = piece;

    // Update material
    pos->material += PSQT[piece][to] - PSQT[piece][from];

    // Update bitboards
    pieceBB(ALL)   ^= SquareBB[from] ^ SquareBB[to];
    pieceBB(pt)    ^= SquareBB[from] ^ SquareBB[to];
    colorBB(color) ^= SquareBB[from] ^ SquareBB[to];
}

// Take back the previous move
void TakeMove(Position *pos) {

    // Decrement histPly, ply
    pos->histPly--;
    pos->ply--;

    // Change side to play
    sideToMove ^= 1;

    // Get the move from history
    const Move move = history(0).move;
    const Square from = fromSq(move);
    const Square to = toSq(move);
#ifdef EVAL_NNUE
    const Piece pc = pieceOn(to);

#endif

    if (promotion(move)) {
#if defined(EVAL_NNUE)
        PieceNumber piece_no0 = pos->state()->dirtyPiece.pieceNo[0];
        pos->evalList.put_piece(piece_no0, to, pc);
#endif  // defined(EVAL_NNUE)
    }

    // Add in pawn captured by en passant
    if (moveIsEnPas(move))
        AddPiece(pos, to ^ 8, MakePiece(!sideToMove, PAWN), false);

    // Move rook back if castling
    else if (moveIsCastle(move)) {
#ifndef EVAL_NNUE
        switch (to) {
            case C1: MovePiece(pos, D1, A1, false); break;
            case C8: MovePiece(pos, D8, A8, false); break;
            case G1: MovePiece(pos, F1, H1, false); break;
            default: MovePiece(pos, F8, H8, false); break;
        }
#else
        Square rto, rfrom;
        switch (to) {
            case C1: rto = D1; rfrom = A1; MovePiece(pos, D1, A1, false); break;
            case C8: rto = D8; rfrom = A8; MovePiece(pos, D8, A8, false); break;
            case G1: rto = F1; rfrom = H1; MovePiece(pos, F1, H1, false); break;
            default: rto = F8; rfrom = H8; MovePiece(pos, F8, H8, false); break;
        }
        auto& dp = pos->state()->dirtyPiece;
        dp.dirty_num = 2;

        PieceNumber piece_no1 = pos->piece_no_of(rto);

        pos->evalList.piece_no_list_board[rto] = PIECE_NUMBER_NB;
        pos->evalList.put_piece(piece_no1, rfrom, MakePiece(sideToMove, ROOK));
#endif  // defined(EVAL_NNUE)
    }

    // Make reverse move (from <-> to)
    MovePiece(pos, to, from, false);

#if defined(EVAL_NNUE)
    PieceNumber piece_no0 = pos->state()->dirtyPiece.pieceNo[0];
    pos->evalList.put_piece(piece_no0, from, pc);
    pos->evalList.piece_no_list_board[to] = PIECE_NUMBER_NB;
#endif  // defined(EVAL_NNUE)

    // Add back captured piece if any
    Piece capt = capturing(move);
    if (capt != EMPTY) {
        assert(ValidCapture(capt));
        AddPiece(pos, to, capt, false);

#if defined(EVAL_NNUE)
        PieceNumber piece_no1 = pos->state()->dirtyPiece.pieceNo[1];
        assert(pos->evalList.bona_piece(piece_no1).fw == Eval::BONA_PIECE_ZERO);
        assert(pos->evalList.bona_piece(piece_no1).fb == Eval::BONA_PIECE_ZERO);
        pos->evalList.put_piece(piece_no1, !moveIsEnPas(move) ? to : to ^ 8, capt);
#endif  // defined(EVAL_NNUE)
    }

    // Remove promoted piece and put back the pawn
    Piece promo = promotion(move);
    if (promo != EMPTY) {
        assert(ValidPromotion(promo));
        ClearPiece(pos, from, false);
        AddPiece(pos, from, MakePiece(sideToMove, PAWN), false);
    }

    // Get various info from history
    pos->key            = history(0).posKey;
    pos->epSquare       = history(0).epSquare;
    pos->rule50         = history(0).rule50;
    pos->castlingRights = history(0).castlingRights;

    assert(PositionOk(pos));
#if defined(EVAL_NNUE)
    assert(pos->evalList.is_valid(*pos));
#endif  // defined(EVAL_NNUE)
}

// Make a move - take it back and return false if move was illegal
bool MakeMove(Position *pos, const Move move) {

#if defined(EVAL_NNUE)

    pos->state()->accumulator.computed_accumulation = false;
    pos->state()->accumulator.computed_score = false;

    PieceNumber piece_no0 = PIECE_NUMBER_NB;
    PieceNumber piece_no1 = PIECE_NUMBER_NB;

    auto& dp = pos->state()->dirtyPiece;
    dp.dirty_num = 1;

    Square capsq = SQUARE_NB;
#endif  // defined(EVAL_NNUE)

    // Save position
    history(0).posKey         = pos->key;
    history(0).move           = move;
    history(0).epSquare       = pos->epSquare;
    history(0).rule50         = pos->rule50;
    history(0).castlingRights = pos->castlingRights;

    // Increment histPly, ply and 50mr
    pos->histPly++;
    pos->ply++;
    pos->rule50++;

    // Hash out en passant if there was one, and unset it
    HASH_EP;
    pos->epSquare = 0;

    const Square from = fromSq(move);
    const Square to = toSq(move);
#ifdef EVAL_NNUE
    const Piece pc = pieceOn(from);
#endif

    // Rehash the castling rights
    HASH_CA;
    pos->castlingRights &= CastlePerm[from] & CastlePerm[to];
    HASH_CA;

    // Remove captured piece if any
    Piece capt = capturing(move);
    if (capt != EMPTY) {
        assert(ValidCapture(capt));
#if defined(EVAL_NNUE)
        capsq = to;
        piece_no1 = pos->piece_no_of(to);
#endif  // defined(EVAL_NNUE)
        ClearPiece(pos, to, true);
        pos->rule50 = 0;
    }

#if defined(EVAL_NNUE)
    piece_no0 = pos->piece_no_of(from);
#endif  // defined(EVAL_NNUE)

    // Move the piece
    MovePiece(pos, from, to, true);

#if defined(EVAL_NNUE)
    dp.pieceNo[0] = piece_no0;
    dp.changed_piece[0].old_piece = pos->evalList.bona_piece(piece_no0);
    pos->evalList.piece_no_list_board[from] = PIECE_NUMBER_NB;
    pos->evalList.put_piece(piece_no0, to, pc);
    dp.changed_piece[0].new_piece = pos->evalList.bona_piece(piece_no0);
#endif  // defined(EVAL_NNUE)

    // Pawn move specifics
    if (PieceTypeOf(pieceOn(to)) == PAWN) {

        pos->rule50 = 0;
        Piece promo = promotion(move);

        // Set en passant square if applicable
        if (moveIsPStart(move)) {
            if ((  PawnAttackBB(sideToMove, to ^ 8)
                 & colorPieceBB(!sideToMove, PAWN))) {

                pos->epSquare = to ^ 8;
                HASH_EP;
            }

        // Remove pawn captured by en passant
        } else if (moveIsEnPas(move)) {
#if defined(EVAL_NNUE)
            capsq = to ^ 8;
            piece_no1 = pos->piece_no_of(capsq);
            pos->evalList.piece_no_list_board[capsq] = PIECE_NUMBER_NB;
#endif  // defined(EVAL_NNUE)
            ClearPiece(pos, to ^ 8, true);

        // Replace promoting pawn with new piece
        } else if (promo != EMPTY) {
            assert(ValidPromotion(promo));
            ClearPiece(pos, to, true);
            AddPiece(pos, to, promo, true);

#if defined(EVAL_NNUE)
            piece_no0 = pos->piece_no_of(to);
            //dp.pieceNo[0] = piece_no0;
            //dp.changed_piece[0].old_piece = pos->evalList.bona_piece(piece_no0);
            assert(pos->evalList.piece_no_list_board[from] == PIECE_NUMBER_NB);
            pos->evalList.put_piece(piece_no0, to, promo);
            dp.changed_piece[0].new_piece = pos->evalList.bona_piece(piece_no0);
#endif  // defined(EVAL_NNUE)
        }

    // Move the rook during castling
    } else if (moveIsCastle(move)) {
#ifndef EVAL_NNUE
        switch (to) {
            case C1: MovePiece(pos, A1, D1, true); break;
            case C8: MovePiece(pos, A8, D8, true); break;
            case G1: MovePiece(pos, H1, F1, true); break;
            default: MovePiece(pos, H8, F8, true); break;
        }
#else
        Square rto;
        switch (to) {
            case C1: capsq = A1; rto = D1; piece_no1 = pos->piece_no_of(capsq); MovePiece(pos, A1, D1, true); break;
            case C8: capsq = A8; rto = D8; piece_no1 = pos->piece_no_of(capsq); MovePiece(pos, A8, D8, true); break;
            case G1: capsq = H1; rto = F1; piece_no1 = pos->piece_no_of(capsq); MovePiece(pos, H1, F1, true); break;
            default: capsq = H8; rto = F8; piece_no1 = pos->piece_no_of(capsq); MovePiece(pos, H8, F8, true); break;
        }
        dp.dirty_num = 2;
        dp.pieceNo[1] = piece_no1;
        dp.changed_piece[1].old_piece = pos->evalList.bona_piece(piece_no1);
        pos->evalList.piece_no_list_board[capsq] = PIECE_NUMBER_NB;
        pos->evalList.put_piece(piece_no1, rto, MakePiece(sideToMove, ROOK));
        dp.changed_piece[1].new_piece = pos->evalList.bona_piece(piece_no1);
#endif  // defined(EVAL_NNUE)
    }

    // Change turn to play
    sideToMove ^= 1;
    HASH_SIDE;

#if defined(EVAL_NNUE)
    if (capt || moveIsEnPas(move)) {
        assert(capsq != SQUARE_NB);
        dp.dirty_num = 2; // ���������2��

        dp.pieceNo[1] = piece_no1;
        dp.changed_piece[1].old_piece = pos->evalList.bona_piece(piece_no1);
        // Do not use Eval::EvalList::put_piece() because the piece is removed
        // from the game, and the corresponding elements of the piece lists
        // needs to be Eval::BONA_PIECE_ZERO.
        pos->evalList.set_piece_on_board(piece_no1, Eval::BONA_PIECE_ZERO, Eval::BONA_PIECE_ZERO, capsq);
        // Set PIECE_NUMBER_NB to pos->piece_no_of_board[capsq] directly because it
        // will not be overritten to pc if the move type is enpassant.
        pos->evalList.piece_no_list_board[capsq] = PIECE_NUMBER_NB;
        dp.changed_piece[1].new_piece = pos->evalList.bona_piece(piece_no1);
    }
#endif  // defined(EVAL_NNUE)

    // If own king is attacked after the move, take it back immediately
    if (KingAttacked(pos, sideToMove^1))
        return TakeMove(pos), false;

    assert(PositionOk(pos));
#if defined(EVAL_NNUE)
     assert(pos->evalList.is_valid(*pos));
#endif  // defined(EVAL_NNUE)

    return true;
}

// Pass the turn without moving
void MakeNullMove(Position *pos) {

    // Save misc info for takeback
    history(0).posKey         = pos->key;
    history(0).move           = NOMOVE;
    history(0).epSquare       = pos->epSquare;
    history(0).rule50         = pos->rule50;
    history(0).castlingRights = pos->castlingRights;

    // Increase ply
    pos->ply++;
    pos->histPly++;

    pos->rule50 = 0;

    // Change side to play
    sideToMove ^= 1;
    HASH_SIDE;

    // Hash out en passant if there was one, and unset it
    HASH_EP;
    pos->epSquare = 0;

    assert(PositionOk(pos));
}

// Take back a null move
void TakeNullMove(Position *pos) {

    // Decrease ply
    pos->histPly--;
    pos->ply--;

    // Change side to play
    sideToMove ^= 1;

    // Get info from history
    pos->key            = history(0).posKey;
    pos->epSquare       = history(0).epSquare;
    pos->rule50         = history(0).rule50;
    pos->castlingRights = history(0).castlingRights;

    assert(PositionOk(pos));
}

#if defined(EVAL_NNUE)
PieceNumber Position::piece_no_of(Square sq) const
{
  assert(piece_on(sq) != NO_PIECE);
  PieceNumber n = evalList.piece_no_of_board(sq);
  assert(is_ok(n));
  return n;
}
#endif  // defined(EVAL_NNUE)
