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

#include "types.h"

class Position;

// Check for (likely) material draw
#define CHECK_MAT_DRAW


#define MakeScore(mg, eg) ((int)((unsigned int)(eg) << 16) + (mg))
#define S(mg, eg) MakeScore((mg), (eg))
#define MgScore(s) ((int16_t)((uint16_t)((unsigned)((s)))))
#define EgScore(s) ((int16_t)((uint16_t)((unsigned)((s) + 0x8000) >> 16)))

#ifdef DEV
#define tuneable_const
#define tuneable_static_const
#else
#define tuneable_const const
#define tuneable_static_const static const
#endif


extern tuneable_const int Tempo;

extern tuneable_const int PieceTypeValue[7];
extern tuneable_const int PieceValue[2][PIECE_NB];


typedef struct EvalInfo {

    Bitboard mobilityArea[2];

} EvalInfo;

namespace Eval {

Value evaluate(const Position *pos);

#if defined(EVAL_NNUE) || defined(EVAL_LEARN)
// �]���֐��t�@�C����ǂݍ��ށB
// ����́A"is_ready"�R�}���h�̉�������1�x�����Ăяo�����B2�x�Ăяo�����Ƃ͑z�肵�Ă��Ȃ��B
// (�������AEvalDir(�]���֐��t�H���_)���ύX�ɂȂ������ƁAisready���ēx�����Ă�����ǂ݂Ȃ����B)
void load_eval();

// --- �]���֐��Ŏg���萔 KPP(�ʂƔC��2��)��P�ɑ�������enum

// (�]���֐��̎����̂Ƃ��ɂ́ABonaPiece�͎��R�ɒ�`�������̂ł����ł͒�`���Ȃ��B)


// Bonanza��KKP/KPP�ƌ����Ƃ���P(Piece)��\������^�B
// �� KPP�����߂�Ƃ��ɁA39�̒n�_�̕��̂悤�ɁA���~���ɑ΂��Ĉ�ӂȔԍ����K�v�ƂȂ�B
enum BonaPiece : int32_t
{
	// f = friend(�����)�̈Ӗ��Be = enemy(�����)�̈Ӗ�

	// ���������̎��̒l
	BONA_PIECE_NOT_INIT = -1,

	// �����ȋ�B����̂Ƃ��Ȃǂ́A�s�v�ȋ�������Ɉړ�������B
	BONA_PIECE_ZERO = 0,

	fe_hand_end = BONA_PIECE_ZERO + 1,

    // Bonanza�̂悤�ɔՏ�̂��肦�Ȃ����̕��⍁�̔ԍ����l�߂Ȃ��B
	// ���R1) �w�K�̂Ƃ��ɑ���PP��1�i�ڂɍ�������Ƃ��������āA������t�ϊ��ɂ����Đ������\������̂�����B
	// ���R2) �c�^Bitboard����Square����̕ϊ��ɍ���B

	// --- �Տ�̋�
	f_pawn = fe_hand_end,
	e_pawn = f_pawn + SQUARE_NB,
	f_knight = e_pawn + SQUARE_NB,
	e_knight = f_knight + SQUARE_NB,
	f_bishop = e_knight + SQUARE_NB,
	e_bishop = f_bishop + SQUARE_NB,
	f_rook = e_bishop + SQUARE_NB,
	e_rook = f_rook + SQUARE_NB,
	f_queen = e_rook + SQUARE_NB,
	e_queen = f_queen + SQUARE_NB,
	fe_end = e_queen + SQUARE_NB,
	f_king = fe_end,
	e_king = f_king + SQUARE_NB,
	fe_end2 = e_king + SQUARE_NB, // �ʂ��܂߂������̔ԍ��B
};

#define ENABLE_INCR_OPERATORS_ON(T)                                \
inline T& operator++(T& d) { return d = T(int(d) + 1); }           \
inline T& operator--(T& d) { return d = T(int(d) - 1); }

ENABLE_INCR_OPERATORS_ON(BonaPiece)

#undef ENABLE_INCR_OPERATORS_ON

// BonaPiece����肩�猩���Ƃ�(����39�̕�����肩�猩��ƌ���71�̕�)�̔ԍ��Ƃ�
// �y�A�ɂ������̂�ExtBonaPiece�^�ƌĂԂ��Ƃɂ���B
union ExtBonaPiece
{
	struct {
		BonaPiece fw; // from white
		BonaPiece fb; // from black
	};
	BonaPiece from[2];

	ExtBonaPiece() {}
	ExtBonaPiece(BonaPiece fw_, BonaPiece fb_) : fw(fw_), fb(fb_) {}
};

// �����̎w����ɂ���Ăǂ�����ǂ��Ɉړ������̂��̏��B
// ���ExtBonaPiece�\���ł���Ƃ���B
struct ChangedBonaPiece
{
	ExtBonaPiece old_piece;
	ExtBonaPiece new_piece;
};

// KPP�e�[�u���̔Տ�̋�pc�ɑΉ�����BonaPiece�����߂邽�߂̔z��B
// ��)
// BonaPiece fb = kpp_board_index[pc].fb + sq; // ��肩�猩��sq�ɂ���pc�ɑΉ�����BonaPiece
// BonaPiece fw = kpp_board_index[pc].fw + sq; // ��肩�猩��sq�ɂ���pc�ɑΉ�����BonaPiece
extern ExtBonaPiece kpp_board_index[PIECE_NB];

// �]���֐��ŗp�����X�g�B�ǂ̋�(PieceNumber)���ǂ��ɂ���̂�(BonaPiece)��ێ����Ă���\����
struct EvalList
{
	// �]���֐�(FV38�^)�ŗp�����ԍ��̃��X�g
	BonaPiece* piece_list_fw() const { return const_cast<BonaPiece*>(pieceListFw); }
	BonaPiece* piece_list_fb() const { return const_cast<BonaPiece*>(pieceListFb); }

	// �w�肳�ꂽpiece_no�̋��ExtBonaPiece�^�ɕϊ����ĕԂ��B
	ExtBonaPiece bona_piece(PieceNumber piece_no) const
	{
		ExtBonaPiece bp;
		bp.fw = pieceListFw[piece_no];
		bp.fb = pieceListFb[piece_no];
		return bp;
	}

	// �Տ��sq�̏���piece_no��pc�̋��z�u����
	void put_piece(PieceNumber piece_no, Square sq, Piece pc) {
		set_piece_on_board(piece_no, BonaPiece(kpp_board_index[pc].fw + sq), BonaPiece(kpp_board_index[pc].fb + Inv(sq)), sq);
	}

	// �Տ�̂��鏡sq�ɑΉ�����PieceNumber��Ԃ��B
	PieceNumber piece_no_of_board(Square sq) const { return piece_no_list_board[sq]; }

	// pieceList������������B
	// ����ɑΉ������鎞�̂��߂ɁA���g�p�̋�̒l��BONA_PIECE_ZERO�ɂ��Ă����B
	// �ʏ�̕]���֐�������̕]���֐��Ƃ��ė��p�ł���B
	// piece_no_list�̂ق��̓f�o�b�O������悤��PIECE_NUMBER_NB�ŏ������B
	void clear()
	{

		for (auto& p : pieceListFw)
			p = BONA_PIECE_ZERO;

		for (auto& p : pieceListFb)
			p = BONA_PIECE_ZERO;

		for (auto& v : piece_no_list_board)
			v = PIECE_NUMBER_NB;
	}

	// �����ŕێ����Ă���pieceListFw[]��������BonaPiece�ł��邩����������B
	// �� : �f�o�b�O�p�B�x���B
	bool is_valid(const Position& pos);

	// �Տ�sq�ɂ���piece_no�̋��BonaPiece��fb,fw�ł��邱�Ƃ�ݒ肷��B
	inline void set_piece_on_board(PieceNumber piece_no, BonaPiece fw, BonaPiece fb, Square sq)
	{
		assert(is_ok(piece_no));
		pieceListFw[piece_no] = fw;
		pieceListFb[piece_no] = fb;
		piece_no_list_board[sq] = piece_no;
	}

	// ��X�g�B��ԍ�(PieceNumber)�����̋�ǂ��ɂ���̂�(BonaPiece)�������BFV38�Ȃǂŗp����B

	// ��X�g�̒���
  // 38�Œ�
public:
	int length() const { return PIECE_NUMBER_KING; }

	// VPGATHERDD���g���s���A4�̔{���łȂ���΂Ȃ�Ȃ��B
	// �܂��AKPPT�^�]���֐��Ȃǂ́A39,40�Ԗڂ̗v�f���[���ł��邱�Ƃ�O��Ƃ���
	// �A�N�Z�X�����Ă���ӏ�������̂Œ��ӂ��邱�ƁB
	static const int MAX_LENGTH = 32;

  // �Տ�̋�ɑ΂��āA���̋�ԍ�(PieceNumber)��ێ����Ă���z��
  // �ʂ�SQUARE_NB�Ɉړ����Ă���Ƃ��p��+1�܂ŕێ����Ă������A
  // SQUARE_NB�̋ʂ��ړ������Ȃ��̂ŁA���̒l���g�����Ƃ͂Ȃ��͂��B
  PieceNumber piece_no_list_board[SQUARE_NB_PLUS1];
private:

	BonaPiece pieceListFw[MAX_LENGTH];
	BonaPiece pieceListFb[MAX_LENGTH];
};

// �]���l�̍����v�Z�̊Ǘ��p
// �O�̋ǖʂ���ړ�������ԍ����Ǘ����邽�߂̍\����
// ������́A�ő��2�B
struct DirtyPiece
{
	// ���̋�ԍ��̋�����牽�ɕς�����̂�
	Eval::ChangedBonaPiece changed_piece[2];

	// dirty�ɂȂ�����ԍ�
	PieceNumber pieceNo[2];

	// dirty�ɂȂ������B
	// null move����0�Ƃ������Ƃ����肤��B
	// ������Ǝ�����Ƃōő��2�B
	int dirty_num;

};
#endif  // defined(EVAL_NNUE) || defined(EVAL_LEARN)

}
