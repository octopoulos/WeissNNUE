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

#pragma once

#include <inttypes.h>
#include <pthread.h>
#include <setjmp.h>
#include <stdbool.h>
#include <cstdint>

#define NDEBUG
#include <assert.h>

#ifdef EVAL_NNUE
#include <immintrin.h>
#endif

// Macro for printing size_t
#ifdef _WIN32
#  ifdef _WIN64
#    define PRI_SIZET PRIu64
#  else
#    define PRI_SIZET PRIu32
#  endif
#else
#  define PRI_SIZET "zu"
#endif

#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

#define INLINE static inline __attribute__((always_inline))
#define CONSTR static __attribute__((constructor)) void

#define lastMoveNullMove (!root && history(-1).move == NOMOVE)
#define history(offset) (pos->gameHistory[pos->histPly + offset])
#define killer1 (thread->killers[pos->ply][0])
#define killer2 (thread->killers[pos->ply][1])

#define pieceBB(type) (pos->pieceBB[(type)])
#define colorBB(color) (pos->colorBB[(color)])
#define colorPieceBB(color, type) (colorBB(color) & pieceBB(type))
#define sideToMove (pos->stm)
#define pieceOn(sq) (pos->board[sq])


typedef uint64_t Bitboard;
typedef uint64_t Key;

typedef uint32_t Move;
typedef uint32_t Square;

typedef int64_t TimePoint;

typedef int32_t Depth;
typedef int32_t Color;
typedef int32_t Piece;
typedef int32_t PieceType;

enum Limit {
    MAXGAMEMOVES     = 512,
    MAXPOSITIONMOVES = 256,
    MAXDEPTH         = 128
};

enum Score {
    TBWIN        = 30000,
    TBWIN_IN_MAX = TBWIN - 999,

    MATE        = 31000,
    MATE_IN_MAX = MATE - 999,

    INFINITE = MATE + 1,
    NOSCORE  = MATE + 2,
};

enum {
    BLACK, WHITE
};

constexpr Color Colors[2] = { WHITE, BLACK };

enum {
    ALL, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, TYPE_NB = 8
};

enum {
    EMPTY, NO_PIECE = 0,
    bP = 1, bN, bB, bR, bQ, bK,
    wP = 9, wN, wB, wR, wQ, wK,
    PIECE_NB = 16
};

enum {
    MG, EG
};

enum {
    P_MG =  110, P_EG =  155,
    N_MG =  437, N_EG =  448,
    B_MG =  460, B_EG =  465,
    R_MG =  670, R_EG =  755,
    Q_MG = 1400, Q_EG = 1560
};

enum File {
    FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_NB
};

enum Rank {
    RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_NB
};

enum {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8, SQUARE_NB, SQUARE_NB_PLUS1, SQUARE_ZERO = 0
};

typedef enum Direction {
    NORTH = 8,
    EAST  = 1,
    SOUTH = -NORTH,
    WEST  = -EAST
} Direction;

enum CastlingRights {
    WHITE_OO  = 1,
    WHITE_OOO = 2,
    BLACK_OO  = 4,
    BLACK_OOO = 8,

    OO  = WHITE_OO  | BLACK_OO,
    OOO = WHITE_OOO | BLACK_OOO,
    WHITE_CASTLE = WHITE_OO | WHITE_OOO,
    BLACK_CASTLE = BLACK_OO | BLACK_OOO
};

constexpr int MAX_MOVES = 256;
constexpr int MAX_PLY   = 246;
enum Value : int {
  VALUE_ZERO      = 0,
  VALUE_DRAW      = 0,
  VALUE_KNOWN_WIN = 10000,
  VALUE_MATE      = 32000,
  VALUE_INFINITE  = 32001,
  VALUE_NONE      = 32002,

  VALUE_TB_WIN_IN_MAX_PLY  =  VALUE_MATE - 2 * MAX_PLY,
  VALUE_TB_LOSS_IN_MAX_PLY = -VALUE_TB_WIN_IN_MAX_PLY,
  VALUE_MATE_IN_MAX_PLY  =  VALUE_MATE - MAX_PLY,
  VALUE_MATED_IN_MAX_PLY = -VALUE_MATE_IN_MAX_PLY,

  PawnValueMg   = 124,   PawnValueEg   = 206,
  KnightValueMg = 781,   KnightValueEg = 854,
  BishopValueMg = 825,   BishopValueEg = 915,
  RookValueMg   = 1276,  RookValueEg   = 1380,
  QueenValueMg  = 2538,  QueenValueEg  = 2682,

  MidgameLimit  = 15258, EndgameLimit  = 3915,

  // �]���֐��̕Ԃ��l�̍ő�l(2**14���炢�Ɏ��܂��Ă��ė~�����Ƃ��낾��..)
  VALUE_MAX_EVAL = 27000,
};

#define ENABLE_BASE_OPERATORS_ON(T)                                \
constexpr T operator+(T d1, T d2) { return T(int(d1) + int(d2)); } \
constexpr T operator-(T d1, T d2) { return T(int(d1) - int(d2)); } \
constexpr T operator-(T d) { return T(-int(d)); }                  \
inline T& operator+=(T& d1, T d2) { return d1 = d1 + d2; }         \
inline T& operator-=(T& d1, T d2) { return d1 = d1 - d2; }

#define ENABLE_INCR_OPERATORS_ON(T)                                \
inline T& operator++(T& d) { return d = T(int(d) + 1); }           \
inline T& operator--(T& d) { return d = T(int(d) - 1); }

#define ENABLE_FULL_OPERATORS_ON(T)                                \
ENABLE_BASE_OPERATORS_ON(T)                                        \
constexpr T operator*(int i, T d) { return T(i * int(d)); }        \
constexpr T operator*(T d, int i) { return T(int(d) * i); }        \
constexpr T operator/(T d, int i) { return T(int(d) / i); }        \
constexpr int operator/(T d1, T d2) { return int(d1) / int(d2); }  \
inline T& operator*=(T& d, int i) { return d = T(int(d) * i); }    \
inline T& operator/=(T& d, int i) { return d = T(int(d) / i); }

ENABLE_FULL_OPERATORS_ON(Value)

#undef ENABLE_FULL_OPERATORS_ON
#undef ENABLE_INCR_OPERATORS_ON
#undef ENABLE_BASE_OPERATORS_ON

/* Structs */

typedef struct PV {
    int length;
    Move line[MAXDEPTH];
} PV;

typedef struct {
    Move move;
    int score;
} MoveListEntry;

typedef struct {
    int count;
    int next;
    MoveListEntry moves[MAXPOSITIONMOVES];
} MoveList;

typedef struct {

    TimePoint start;
    int time, inc, movestogo, movetime, depth;
    int optimalUsage, maxUsage;
    bool timelimit, infinite;

} SearchLimits;

#if defined(EVAL_NNUE) || defined(EVAL_LEARN)
// --------------------
//        �
// --------------------

constexpr Square make_square(File f, Rank r) {
  return Square((r << 3) + f);
}

constexpr File file_of(Square s) {
  return File(s & 7);
}

constexpr Rank rank_of(Square s) {
  return Rank(s >> 3);
}

constexpr PieceType type_of(Piece pc) {
  return PieceType(pc & 7);
}

// �Ֆʂ�180���񂵂��Ƃ��̏��ڂ�Ԃ�
constexpr Square Inv(Square sq) { return (Square)((SQUARE_NB - 1) - sq); }

// �Ֆʂ��~���[�����Ƃ��̏��ڂ�Ԃ�
constexpr Square Mir(Square sq) { return make_square(File(7 - (int)file_of(sq)), rank_of(sq)); }

// Position�N���X�ŗp����A��X�g(�ǂ̋�ǂ��ɂ���̂�)���Ǘ�����Ƃ��̔ԍ��B
enum PieceNumber : uint8_t
{
	PIECE_NUMBER_PAWN = 0,
	PIECE_NUMBER_KNIGHT = 16,
	PIECE_NUMBER_BISHOP = 20,
	PIECE_NUMBER_ROOK = 24,
	PIECE_NUMBER_QUEEN = 28,
	PIECE_NUMBER_KING = 30,
	PIECE_NUMBER_WKING = 30,
	PIECE_NUMBER_BKING = 31, // ���A���̋ʂ̔ԍ����K�v�ȏꍇ�͂�������p����
	PIECE_NUMBER_ZERO = 0,
	PIECE_NUMBER_NB = 32,
};

inline PieceNumber& operator++(PieceNumber& d) { return d = PieceNumber(int8_t(d) + 1); }
inline PieceNumber operator++(PieceNumber& d, int) {
  PieceNumber x = d;
  d = PieceNumber(int8_t(d) + 1);
  return x;
}
inline PieceNumber& operator--(PieceNumber& d) { return d = PieceNumber(int8_t(d) - 1); }

// PieceNumber�̐������̌����Bassert�p�B
constexpr bool is_ok(PieceNumber pn) { return pn < PIECE_NUMBER_NB; }
#endif  // defined(EVAL_NNUE) || defined(EVAL_LEARN)
