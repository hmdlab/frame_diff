/* -------------------------------------------------------------------------------------
 *
 *	frame_diff_template_basic.c:
 *					電生実験　「フレーム間差分オブジェクト抽出」
 *					実験１・実験２用プログラムコード（ひな形、初心者向け）
 * -------------------------------------------------------------------------------------
 * 【処理の概要】
 *  （１） ３枚の入力画像（カラー、BMP形式、256諧調）を指定されたファイルから読み込む
 *   (1') 合成用背景画像を指定されたファイルから読み込む（オプション'-comosite' 指定時のみ実行）
 *   (1'') 誤差評価用の正解オブジェクト領域画像を読み込む（オプション'-ideal' 指定時のみ実行）
 *  （2） ３枚の入力画像（カラー）それぞれを輝度画像（グレースケール）に変換する
 *  （3） ３枚の輝度画像から２枚の輝度フレーム間差分画像を求め、それぞれのヒストグラムデータを作成する。 ←【各自で作成】
 *  （4） ２枚の輝度フレーム間差分画像を２値化し、２つのオブジェクト領域候補を求める。				←【各自で作成】
 *  （5） ２つのオブジェクト領域候補のANDをとり、オブジェクト領域を求める。							←【各自で作成】
 *  （6） 正解オブジェクト領域に対する誤差を計算する。（オプション'-ideal' 指定時のみ実行）
 *  （7） 合成画像を作成する。（オプション'-composite'指定時間のみ実行）						←【各自で作成】
 *  （8） 処理結果をファイルに出力する
 *
 * 【使用方法】
 *  frame_diff \
 *		-in1 (input bmp 1) \			# 入力画像ファイル１の指定
 *		-in2 (input bmp 2) \			# 入力画像ファイル２の指定
 *		-in3 (input bmp 3) \			# 入力画像ファイル３の指定
 *		-histogram　(histogram data） \   # ヒストグラムデータの出力先の指定
 *		-alpha1 (alpha values 1) \		# オブジェクト領域候補１の出力先の指定
 *		-alpha2 (alpha values 2) \		# オブジェクト領域候補１の出力先の指定
 *		-theta (threshold) \			# ２値化処理の閾値の指定
 *		-out　（output bmp） \				# 処理結果（オブジェクト領域の画像または合成画像）の出力先の指定
 *		[-composite -base (base bmp)] \ # 画像合成の実行の指示と背景画像の指定（オプション）
 *		[-ideal (ideal mask)]			# 誤差評価用の正解オブジェクト領域画像の指定（オプション）
 *
 * ------------------------------------------------------------------------------------- */

#include <string.h>
#include "image_io.h"

#define FORE_GROUND	255		/* オブジェクト領域をあらわすフラグ */
#define BACK_GROUND	0		/* 背景領域をあらわすフラグ */
#define NUM_OF_BINS 64 		/* ヒストグラムのビンの総数 */
#define MAX 255				/* 画像データ（BMP形式）のRGB成分の最大値 */
#define MIN 0				/* 画像データ（BMP形式）のRGB成分の最小値*/
#define BUFSZ 100			/* 文字列（ファイル名）の最大長 */


/* -------------------------------
 *
 *	関数の宣言部
 *
 * ------------------------------- */

/* 引数に関するメッセージの表示 */
void usage();

/* 初期化 */
void initialise(
	int argc,
	char **argv,
	FILE **fp_bmp_in_1,
	FILE **fp_bmp_in_2,
	FILE **fp_bmp_in_3,
	FILE **fp_bmp_base,
	FILE **fp_histogram,
	FILE **fp_bmp_alpha_1,
	FILE **fp_bmp_alpha_2,
	FILE **fp_bmp_actual,
	FILE **fp_bmp_ideal,
	int *h_size,
	int *v_size,
	float *theta,
	int *evaluation,
	int *composite
);

/* 量子化処理（ヒストグラムのビンの決定）*/
int quantize (
	float value
);

/*２値化処理 */
void binarize (
	unsigned char *input_arr,
	unsigned char *output_img[3],
	float theta,
	int h_size,
	int v_size
);

/* 誤差の評価（不一致ピクセル数／総ピクセル数） */
float eval_err_rate (
	unsigned char *actual_img[],
	unsigned char *ideal_img[],
	int h_size,
	int v_size
);



/* -------------------------------
 *
 *	メイン関数
 *
 * ------------------------------- */
int main(int argc,char *argv[])
{
unsigned char
	*input_bmp_image[3][3],		/* 入力画像データ(RGBカラー、t=1,2,3) */
	*input_grey_image[3],		/* 入力画像データ（グレー、t=1,2,3） */
	*base_bmp_image[3],			/* 合成用背景画像データ（RGBカラー） */
	*alpha_values[2],			/* 輝度フレーム間差分のデータ	*/
	*cand_mask[2][3],			/* オブジェクト領域候補（FORE_GROUND：オブジェクト領域,BACK_GROUND：背景） */
	*actual_mask[3],			/* 処理結果（オブジェクト領域画像or 合成画像）*/
	*ideal_mask[3],				/* 処理結果評価用の正解データ（FORE_GROUND：オブジェクト領域, BACK_GROUND：背景） */
	diff;						/* 注目している画素における輝度差分 */

float
	r,							/* 注目画素のR成分（0～255） */
	g,							/* 注目画素のG成分（0～255） */
	b,							/* 注目画素のB成分（0～255） */
	y;							/* 注目画素の輝度値（0～255） */

FILE
	*fp_bmp_in_1,				/* 入力画像ファイル１（BMP形式） */
	*fp_bmp_in_2,				/* 入力画像ファイル２（BMP形式） */
	*fp_bmp_in_3,				/* 入力画像ファイル３（BMP形式） */
	*fp_bmp_base,				/* 合成用背景画像ファイル（BMP形式） */
	*fp_histogram,				/* ヒストグラムデータの出力先（テキストファイル）*/
	*fp_bmp_alpha_1,			/* オブジェクト領域候補画像ファイル１（BMP形式） */
	*fp_bmp_alpha_2,			/* オブジェクト領域候補画像ファイル２（BMP形式） */
	*fp_bmp_actual,				/* オブジェクト領域抽出結果の出力先（BMP形式）*/
	*fp_bmp_ideal;				/* 正解オブジェクト領域画像（BMP形式） */

int
	h_size,						/* 入出力画像のサイズ（ヨコ） */
	v_size,						/* 入出力画像のサイズ（タテ） */
	i, j,						/* 画像平面）上のX-Y座標 */
	t,							/* 入力画像の時刻 */
	c,							/* BMP画像データの色成分のインデックス*/
	k,							/* オブジェクト領域候補の番号 */
	ret,						/* 画像データの読み込み・書き出し操作のチェック（0：失敗, 1:成功） */
	pos,						/* １次元配列上のインデックス */
	bin,						/* ヒストグラムのビンの番号 */
	evaluation,					/* エラー評価処理　０：OFF, １：ON */
	composite,					/* 0：オブジェクト領域を抽出,　 1：合成画像を作成 */
	histograms[2][NUM_OF_BINS], /* 輝度差分のヒストグラム */
	step_size;					/* ヒストグラムの各ビンの幅 */

float
	theta,						/* ２値化の閾値（0～255） */
	err;						/* 注目画素でのオブジェクト領域のエラー */


	/*
	 *  （０）　初期化処理：入出力ファイルのポインタ、画像サイズ、閾値などの設定
	 */
	initialise(
		argc,					/* in：メイン関数の引数の総数 */
		argv,					/* in：メイン関数の引数の実体 */
		&fp_bmp_in_1, 			/* out： 入力画像１のファイルポインタ */
		&fp_bmp_in_2, 			/* out： 入力画像２のファイルポインタ */
		&fp_bmp_in_3,			/* out： 入力画像３のファイルポインタ */
		&fp_bmp_base,			/* out： 合成用背景画像のファイルポインタ */
		&fp_histogram,			/* out： ヒストグラムデータ出力先のファイルポインタ */
		&fp_bmp_alpha_1,		/* out： 候補領域１のファイルポインタ */
		&fp_bmp_alpha_2,		/* out： 候補領域２のファイルポインタ */
		&fp_bmp_actual,			/* out： 処理結果の出力先のファイルポインタ */
		&fp_bmp_ideal,			/* out： 正解データのファイルポインタ */
		&h_size,				/* out： 入力画像のサイズ（ヨコ） */
		&v_size,				/* out： 入力画像のサイズ（タテ） */
		&theta,					/* out： ２値化処理の閾値 */
		&evaluation,			/* out： 誤差の評価に関するフラグ（０：非実行、１：実行） */
		&composite);			/* out： 画像の合成に関するフラグ（０：非実行、１：実行） */

	/* ヒストグラムデータ格納配列の初期化 */
	for (bin=0;bin<NUM_OF_BINS;bin++) {
		histograms[0][bin] = 0;
		histograms[1][bin] = 0;
	}

	/* 画像データ格納用１次元配列のメモリー領域の確保 */
	for (t=0; t<3; t++) {
		/* input_bmp_image[t][c]：時刻t（=0,1,2）における入力画像（RGBカラー画像, c:色成分の番号） */
		for(c=0; c<3; c++)
			input_bmp_image[t][c]=(unsigned char *) malloc (h_size*v_size*sizeof(unsigned char));
		/* input_grey_image[t]：時刻t（=0,1,2）における入力画像の輝度成分 */
		input_grey_image[t]=(unsigned char *) malloc (h_size*v_size*sizeof(unsigned char));
	}
	for (k=0; k<2; k++) {		/* k=0：t=0とt=1の差分, k=1:t=1とt=2の差分*/
		/* alpha_values[k]：k番目の輝度フレーム間差分 */
		alpha_values[k]=(unsigned char *) malloc (h_size*v_size*sizeof(unsigned char));
		/* cand_mask[k][c]：k番目のオブジェクト領域候補（RGBカラー画像, c:色成分の番号） */
		for(c=0; c<3; c++)
			cand_mask[k][c]=(unsigned char *) malloc (h_size*v_size*sizeof(unsigned char));
	}
	for (c=0; c<3; c++) {
		/* actual_mask[c]：オブジェクト領域（RGBカラー画像, c:色成分の番号） */
		actual_mask[c]=(unsigned char *) malloc(h_size*v_size*sizeof(unsigned char));
		/* ideal_mask[c]：正解オブジェクト領域（RGBカラー画像, c:色成分の番号） */
		if (evaluation) {
			ideal_mask[c]=(unsigned char *) malloc(h_size*v_size*sizeof(unsigned char));
		}
		/* base_mask[c]：合成用背景画像（RGBカラー画像, c:色成分の番号） */
		if (composite) {
			base_bmp_image[c]=(unsigned char *) malloc(h_size*v_size*sizeof(unsigned char));
		}
	}


	/*
	 *  （１）　画像データの読み込み
	 */
	ret=image_input_bmp(
		input_bmp_image[0],		/* out：入力画像１(t=0) */
		h_size,					/* in：画像サイズ（ヨコ） */
		v_size,					/* in：画像サイズ（タテ） */
		fp_bmp_in_1);			/* in：画像ファイルのポインタ */

	ret=image_input_bmp(
		input_bmp_image[1],		/* out：入力画像2(t=1) */
		h_size,					/* in：画像サイズ（ヨコ） */
		v_size,					/* in：画像サイズ（タテ） */
		fp_bmp_in_2	);			/* in：画像ファイルのポインタ */

	ret=image_input_bmp(
		input_bmp_image[2],		/* out：入力画像3(t=2) */
		h_size,					/* in：画像サイズ（ヨコ） */
		v_size,					/* in：画像サイズ（タテ） */
		fp_bmp_in_3);			/* in：画像ファイルのポインタ */

	/* (1') 合成用背景画像の読み込み（オプション） */
	if (composite) {
		ret=image_input_bmp(
			base_bmp_image,		/* out：合成用背景画像 */
			h_size,				/* in：画像サイズ（ヨコ） */
			v_size,				/* in：画像サイズ（タテ） */
			fp_bmp_base);		/* in：画像ファイルのポインタ */
	}

	/* (1'') 正解オブジェクト領域画像の読み込み（オプション） */
	if (evaluation) {
		ret=image_input_bmp(
			ideal_mask,			/* out：正解オブジェクト領域画像 */
			h_size,				/* in：画像サイズ（ヨコ） */
			v_size,				/* in：画像サイズ（タテ） */
			fp_bmp_ideal);		/* in：画像ファイルのポインタ */
	}


	/*
	 *  （２）　入力画像の変換（RGBカラー　→　輝度）
	 */
	for(j=0; j<v_size; j++){
		for(i=0; i<h_size; i++){
			pos=j*h_size+i;									/* pos: 画像平面上の点(i,j)に対応する配列のインデックス */
			for(t=0; t<3; t++){
				r=(float)input_bmp_image[t][0][pos];		/* R成分 */
				g=(float)input_bmp_image[t][1][pos];		/* G成分 */
				b=(float)input_bmp_image[t][2][pos];		/* B成分 */
				y=0.257*r+0.504*g+0.098*b+16.0;				/* 輝度値の計算 */
				input_grey_image[t][pos]=(unsigned char)y;	/* 輝度値の格納 */
			}
		}
	}


	/* ====================================================================
	 *
	 *  【実験１】 フレーム間差分法
	 *
	 * --------------------------------------------------------------------
	 *		インデックス（パラメータ）：
	 *			int t;				入力画像の時刻（= 0, 1, 2）
	 *			int h_size;			入力画像のサイズ（ヨコ）
	 *			int v_size;			入力画像のサイズ（タテ）
	 *			int pos;			１次元配列の番号(= 0, ... , h_size*v_size-1)
	 *			int k;				差分画像の番号（= 0, 1）
	 *			int c;				色成分の番号（c=0:R, c=1:G, c=2:B）
	 *			unsigned char diff  輝度差分の絶対値
	 *			int bin;			ヒストグラムのビンの番号
	 *			float theta;		２値化処理の閾値
	 *
	 *		入力データの参照先：
	 *			unsigned char input_grey_image[t][pos]; 入力画像の輝度値（256諧調）
	 *
	 *		計算結果の保存先：
	 *			unsigned char alpha_values[k][pos];		フレーム間差分の絶対値（256諧調）
	 *			unsigned char cand_masks[k][pos];		オブジェクト領域候補（２値）
	 *			unsigned char actual_mask[c][pos];		オブジェクト領域（2値）
	 *			int histograms[k][bin];					フレーム間差分の頻度
	 *
	 *		利用可能な関数：
	 *			float abs(float x);						xの絶対値
	 *			int quantize(unsigned char diff);		diffに対応するビンの番号
	 *			void binarize(unsigned char *input_arr,
	 *						  unsigned char *output_img[3],
	 *						  float theta,
	 *						  int h_size, int v_size);	２値化処理
	 *
	 * -------------------------------------------------------------------- */

	/* (3) 輝度フレーム間差分の計算とヒストグラムデータの作成	・・・　【各自で必要なプログラムコードを作成】 */
	for(j=0; j<v_size; j++) {
		for(i=0; i<h_size; i++) {
			pos=j*h_size+i;		/* pos: 画像平面上の点(i,j)に対応する配列のインデックス */
			/* ---------------------------------------------------------------
			 *						【 ここに追加すべき処理の内容 】
			 * ---------------------------------------------------------------
			 * (3-1) input_grey_image[0][pos] と input_grey_image[1][pos] から
			 *		 輝度差分の絶対値（diff）を計算
			 * (3-2) diffを alpha_values[0][pos] に格納
			 * (3-3) diffの値に対応するビンの番号（bin）を計算
			 * (3-4) histograms[0][bin] の値を1増やす（ビンに対する出現頻度を計算）
			 * (3-5) alpha_values[1][pos], histograms[1][bin]　についても同様に
			 *       input_grey_image[1][pos]とinput_grey_image[2][pos]を元に計算する
			 * --------------------------------------------------------------- */
			/* ここから */


			/* ここまでに記述 */
		} /* for (i) */
	} /* for (j) */



	/* (4) オブジェクト領域候補の計算（２値化）   ・・・　【各自で必要なプログラムコードを作成】 */
	/* --------------------------------------------------------------------
	 *							【 ここに追加すべき処理の内容 】
	 * --------------------------------------------------------------------
	 *  (4-1) 関数 binarize(・・・) を使って alpha_values[0]からcand_mask[0]を計算
	 *  (4-2) cand_mask[1] についても同様に計算
	 * -------------------------------------------------------------------- */
	/* ここから */


	/* ここまでに記述 */

	/* (5) オブジェクト領域の計算（AND処理）   ・・・　【各自で必要なプログラムコードを作成】 */
	for(j=0; j<v_size; j++) {
		for(i=0; i<h_size; i++) {
			pos=j*h_size+i;		/* pos: 画像平面上の点(i,j)に対応する配列のインデックス */

			/* ----------------------------------------------------------------------
			 *							【 ここに追加すべき処理の内容 】
			 * ----------------------------------------------------------------------
			 *  (5-1) cand_mask[0][0][pos]の値がFORE_GROUNDかどうかチェック
			 *        (cand_mask[0][1][pos]やcand_mask[0][2][pos]を使ってもよい)
			 *  (5-2) cand_mask[1][0][pos]の値がFORE_GROUNDかどうかチェック
			 *        (cand_mask[1][1][pos]やcand_mask[1][2][pos]を使ってもよい)
			 *  (5-3) 両者が共に'FORE_GROUND'の場合は、オブジェクト領域画像のRGB成分
			 *		  (actual_mask[0][pos], actual_mask[1][pos], actual_mask[2][pos])
			 *		  の値を全て'FORE_GROUND'に設定。　そうでない場合は'BACK_GROUND'に設定
			 * ---------------------------------------------------------------------- */
			/* ここから */


			/* ここまでに記述 */
		} /* for (i) */
	} /* for (j) */




	/*
	 *  (6) 正解オブジェクト領域に対する誤差の評価（オプション）
	 */
	if (evaluation) {
		err=eval_err_rate(		/* out：オブジェクト領域抽出結果の誤差[%] */
			actual_mask,		/* in：オブジェクト領域 */
			ideal_mask,			/* in：正解オブジェクト領域 */
			h_size,				/* in：画像サイズ（ヨコ） */
			v_size);			/* in：画像サイズ（タテ） */
		fprintf(stderr,"theta=%2.0f , error=%4.3f\n", theta, err);
		fprintf(stdout,"%2.0f	%4.3f\n", theta, err);
	}


	/* ====================================================================
	 *
	 *  【実験２】 画像の合成
	 *
	 * --------------------------------------------------------------------
	 *		インデックス（パラメータ）：
	 *			int h_size;		入力画像のサイズ（ヨコ）
	 *			int v_size;		入力画像のサイズ（タテ）
	 *			int pos;		１次元配列の番号(= 0, ... , h_size*v_size-1)
	 *			int c;			色成分の番号（c=0:R, c=1:G, c=2:B）
	 *
	 *		入力データの参照先：
	 *			unsigned char input_bmp_image[1][c][pos];   時刻t=1の入力画像（256諧調、カラー）
	 *			unsigned char base_bmp_image[c][pos];   合成用背景画像（256諧調、カラー）
	 *			unsigned char actual_mask[c][pos];		オブジェクト領域（２値）
	 *
	 *		計算結果の保存先：
	 *			unsigned char base_bmp_image[c][pos];   合成結果の画像（上書き）（256諧調、カラー）
	 *
	 * -------------------------------------------------------------------- */

	/* (7) 合成画像の作成   ・・・　【各自で必要なプログラムコードを作成】 */
	if (composite) {
		for(j=0; j<v_size; j++) {
			for(i=0; i<h_size; i++) {
				pos=j*h_size+i;		/* pos: 画像平面上の点(i,j)に対応する配列のインデックス */

				/* --------------------------------------------------------------------
				 *							【 ここに追加すべき処理の内容 】
				 * --------------------------------------------------------------------
				 *  (7-1) actual_mask[0][pos]（色を表す0の値は 1,2 のいずれでも可）の値をチェック
				 *  (7-2) チェックした値が'FORE_GROUND'の場合のみ、
				 *		 base_bmp_image[0][pos]をinput_bmp_image[1][0][pos]、
				 * 		 base_bmp_image[1][pos]をinput_bmp_image[1][1][pos]、
				 * 		 base_bmp_image[2][pos]をinput_bmp_image[1][2][pos]の値で上書き
				 * -------------------------------------------------------------------- */

			} /* for(i) */
		} /* for(j) */
	} /* if (composite) */


	/*
	 *  (8) 処理結果の出力
	 */
	if (composite) {
		/* 合成画像の出力（オプション） */
		ret=image_output_bmp(
			base_bmp_image,		/* in：合成画像 */
			h_size,				/* in：画像サイズ（ヨコ） */
			v_size,				/* in：画像サイズ（タテ） */
			fp_bmp_actual);		/* out：合成画像の出力先 */
	} else {
		/* オブジェクト領域画像の出力（デフォルト） */
		ret=image_output_bmp(
			actual_mask,		/* in：オブジェクト領域画像 */
			h_size,				/* in：画像サイズ（ヨコ） */
			v_size,				/* in：画像サイズ（タテ） */
			fp_bmp_actual);		/* out：オブジェクト領域画像の出力先 */
	}

	/* オブジェクト領域候補１の出力 */
	ret=image_output_bmp(
		cand_mask[0],			/* in：オブジェクト領域候補１ */
		h_size,					/* in：画像サイズ（ヨコ） */
		v_size,					/* in：画像サイズ（タテ） */
		fp_bmp_alpha_1);		/* out：オブジェクト領域候補１の出力先 */

	/* オブジェクト領域候補２の出力 */
	ret=image_output_bmp(
		cand_mask[1],			/* in：オブジェクト領域候補２ */
		h_size,					/* in：画像サイズ（ヨコ） */
		v_size,					/* in：画像サイズ（タテ） */
		fp_bmp_alpha_2);		/* out：オブジェクト領域候補２の出力先 */

	/* ヒストグラムデータの出力 */
	step_size= (MAX - MIN + 1) / NUM_OF_BINS;   /* 各ビンの幅 */
	fprintf(fp_histogram,"# MID(bin)  N(|Y2-Y1|,bin)  N(|Y3-Y2|,bin)\n");   /* 凡例 */
	for (bin=0; bin<NUM_OF_BINS; bin++) {
		fprintf(
			fp_histogram,				/* out:ヒストグラムデータの出力先 */
			"%d\t%d\t%d\n",				/* in:データファイルの書式（代表値   頻度１　	頻度２） */
			bin*step_size,              /* in:開始値 */
			histograms[0][bin],			/* in:輝度フレーム間差分１の頻度 */
			histograms[1][bin]			/* in:輝度フレーム間差分２の頻度 */
		);
	}


	/*
	 *  (9) 終了処理（メモリの解放など）
	 */
	fclose(fp_bmp_in_1);
	fclose(fp_bmp_in_2);
	fclose(fp_bmp_in_3);
	fclose(fp_histogram);
	fclose(fp_bmp_alpha_1);
	fclose(fp_bmp_alpha_2);
	fclose(fp_bmp_actual);
	if (composite) fclose(fp_bmp_base);
	if (evaluation) fclose(fp_bmp_ideal);

	for(c=0; c<3; c++) {
		for(t=0; t<3; t++) 	free((unsigned char *) input_bmp_image[t][c]);
		free((unsigned char *) input_grey_image[c]);
		free((unsigned char *) actual_mask[c]);
		for(k=0; k<2; k++) 	free((unsigned char *) cand_mask[k][c]);
		if (evaluation) free((unsigned char *) ideal_mask[c]);
	}
	for (k=0; k<2; k++) {
		free((unsigned char *) alpha_values[k]);
	}
	fprintf(stderr,"program terminated normally\n");

	return 0;
}



/* -------------------------------
 *
 *	サブ関数
 *
 * ------------------------------- */

/* 引数に関するメッセージの表示 */
void usage (char *s)
{
	fprintf(stdout," frame_diff\n");
	fprintf(stdout,"	-in1\t\t(in:  input bmp 1)\n");
	fprintf(stdout,"	-in2\t\t(in:  input bmp 2)\n");
	fprintf(stdout,"	-in3\t\t(in:  input bmp 3)\n");
	fprintf(stdout,"	-histogram\t(out: histogram data)\n");
	fprintf(stdout,"	-alpha1\t\t(out: alpha values 1)\n");
	fprintf(stdout,"	-alpha2\t\t(out: alpha values 2)\n");
	fprintf(stdout,"	-theta\t\t(in:  threshold)\n");
	fprintf(stdout,"	-out\t\t(out: output bmp)\n");
	fprintf(stdout,"	[-composite\t-base (in: base bmp)]\n");
	fprintf(stdout,"	[-ideal\t\t(in:  ideal mask)]\n");

  exit(1);
}

/* 初期化 */
void initialise(
	int argc,
	char **argv,
	FILE **fp_bmp_in_1,
	FILE **fp_bmp_in_2,
	FILE **fp_bmp_in_3,
	FILE **fp_bmp_base,
	FILE **fp_histogram,
	FILE **fp_bmp_alpha_1,
	FILE **fp_bmp_alpha_2,
	FILE **fp_bmp_actual,
	FILE **fp_bmp_ideal,
	int *h_size,
	int *v_size,
	float *theta,
	int *evaluation,
	int *composite
)
{
	int
		ret,
		i,j,k,
		bin,
		h,v;

	int
		fp_in_1_check=0,
		fp_in_2_check=0,
		fp_in_3_check=0,
		fp_base_check=0,
		fp_hist_check=0,
		fp_actual_check=0,
		fp_ideal_check=0,
		fp_alpha_1_check=0,
		fp_alpha_2_check=0;

	char
		fn_bmp_in_1[BUFSZ],
		fn_bmp_in_2[BUFSZ],
		fn_bmp_in_3[BUFSZ],
		fn_bmp_base[BUFSZ],
		fn_bmp_alpha_1[BUFSZ],
		fn_bmp_alpha_2[BUFSZ],
		fn_bmp_actual[BUFSZ],
		fn_bmp_ideal[BUFSZ],
		fn_histogram[BUFSZ],
		*string;


	/* 各オプションの設定（デフォルト値） */
	*evaluation=0;
	*composite=0;

	/* 引数の読み込み */
	i=1;
	while (i<argc) {
		if (argv[i][0]=='-') {
			string = argv[i];

			if (strcmp( string, "-in1" ) == 0 ) {
				  fp_in_1_check=1;
				  sprintf(fn_bmp_in_1,"%s",argv[i+1]);
				  i+=2;
			} else if (strcmp( string, "-in2" ) == 0 ) {
				  fp_in_2_check=1;
				  sprintf(fn_bmp_in_2,"%s",argv[i+1]);
				  i+=2;
			} else if (strcmp( string, "-in3" ) == 0 ) {
				  fp_in_3_check=1;
				  sprintf(fn_bmp_in_3,"%s",argv[i+1]);
				  i+=2;
			} else if (strcmp( string, "-base" ) == 0 ) {
				  fp_base_check=1;
				  *composite=1;
				  sprintf(fn_bmp_base,"%s",argv[i+1]);
				  i+=2;
			} else if (strcmp( string, "-histogram" ) == 0 ) {
				  fp_hist_check=1;
				  sprintf(fn_histogram,"%s",argv[i+1]);
				  i+=2;
			} else if(strcmp( string, "-alpha1" ) == 0 ) {
				  fp_alpha_1_check=1;
				  sprintf(fn_bmp_alpha_1,"%s",argv[i+1]);
				  i+=2;
			} else if(strcmp( string, "-alpha2" ) == 0 ) {
				  fp_alpha_2_check=1;
				  sprintf(fn_bmp_alpha_2,"%s",argv[i+1]);
				  i+=2;
			} else if(strcmp( string, "-out" ) == 0 ) {
				  fp_actual_check=1;
				  sprintf(fn_bmp_actual,"%s",argv[i+1]);
				  i+=2;
			} else if(strcmp( string, "-ideal" ) == 0 ) {
				  fp_ideal_check=1;
				  *evaluation=1;
				  sprintf(fn_bmp_ideal,"%s",argv[i+1]);
				  i+=2;
			} else if(strcmp( string, "-theta" ) == 0 ) {
				  *theta = atof(argv[i+1]);
				  i+=2;
			} else {
				  fprintf(stderr,"unknown option %s \n",string);
				  exit(0);
			}
		} else {
			fprintf(stderr,"unknown option %s \n",argv[i]);
			exit(0);
		}
	}


	/* 入出力先（ファイル）の検査 */
	if (fp_in_1_check==0) {
		fprintf(stderr," set input file name(t=1) with -in1 \n");
		usage(argv[0]);
	} else {
		if ((*fp_bmp_in_1=fopen(fn_bmp_in_1,"rb"))==NULL) {
			fprintf(stderr," Can't open input file-1:%s\n", fn_bmp_in_1);
			exit(1);
		}
		fprintf(stderr," input bmp file(1): %s\n",fn_bmp_in_1);
	}

	if (fp_in_2_check==0) {
			fprintf(stderr," set input file name(t=2) with -in2 \n");
			usage(argv[0]);
	} else {
			if ((*fp_bmp_in_2=fopen(fn_bmp_in_2,"rb"))==NULL) {
			fprintf(stderr," Can't open input file-2:%s\n", fn_bmp_in_2);
			exit(1);
		}
		fprintf(stderr," input bmp file(2): %s\n",fn_bmp_in_2);
	}

	if (fp_in_3_check==0) {
		fprintf(stderr," set input file name(t=3) with -in3 \n");
		usage(argv[0]);
	} else {
		if ((*fp_bmp_in_3=fopen(fn_bmp_in_3,"rb"))==NULL) {
			fprintf(stderr," Can't open input file-3:%s\n", fn_bmp_in_3);
			exit(1);
		}
		fprintf(stderr," input bmp file(3): %s\n",fn_bmp_in_3);
	}

	if (*composite) {
		if (fp_base_check==0) {
			fprintf(stderr," set base image file with -base \n");
			usage(argv[0]);
		} else {
			if ((*fp_bmp_base=fopen(fn_bmp_base,"rb"))==NULL) {
				fprintf(stderr," Can't open base image file:%s\n", fn_bmp_base);
				exit(1);
			}
			fprintf(stderr," base bmp file: %s\n",fn_bmp_base);
		}
	}

	if (fp_hist_check==0) {
			fprintf(stderr," set output file name (histogram) with -histogram \n");
			usage(argv[0]);
	} else {
			if ((*fp_histogram=fopen(fn_histogram,"wb"))==NULL) {
			fprintf(stderr," Can't open file (histogram):%s\n", fn_histogram);
			exit(1);
		}
		fprintf(stderr," histogram: %s\n",fn_histogram);
	}

	if (fp_alpha_1_check==0) {
			fprintf(stderr," set output file name (alpha values) with -alpha1\n");
			usage(argv[0]);

	} else {
		if ((*fp_bmp_alpha_1=fopen(fn_bmp_alpha_1,"wb"))==NULL) {
			fprintf(stderr," Can't open file (alpha values): %s\n", fn_bmp_alpha_1);
			exit(1);
		}
		fprintf(stderr," alpha values(1): %s\n",fn_bmp_alpha_1);
	}

	if (fp_alpha_2_check==0) {
			fprintf(stderr," set output file name (alpha values) with -alpha2\n");
			usage(argv[0]);

	} else {
		if ((*fp_bmp_alpha_2=fopen(fn_bmp_alpha_2,"wb"))==NULL) {
			fprintf(stderr," Can't open file (alpha values): %s\n", fn_bmp_alpha_2);
			exit(1);
		}
		fprintf(stderr," alpha values(2): %s\n",fn_bmp_alpha_2);
	}

	if (fp_actual_check==0) {
			fprintf(stderr," set output file name (actual mask image) with -out\n");
			usage(argv[0]);

	} else {
		if ((*fp_bmp_actual=fopen(fn_bmp_actual,"wb"))==NULL) {
			fprintf(stderr," Can't open file (actual mask image): %s\n", fn_bmp_actual);
			exit(1);
		}
		fprintf(stderr," output file (mask image): %s\n",fn_bmp_actual);
	}

	if (fp_ideal_check==0) {
		if (*evaluation) {
			fprintf(stderr," set input file name (ideal mask image) with -ideal\n");
			usage(argv[0]);
		}
	} else {
		if ((*fp_bmp_ideal=fopen(fn_bmp_ideal,"rb"))==NULL) {
			fprintf(stderr," Can't open file (ideal mask image): %s\n", fn_bmp_ideal);
			exit(1);
		}
		fprintf(stderr," ground truth (mask image): %s\n",fn_bmp_ideal);
	}


	/* 入力画像１の取得 */
	ret=get_bmp_image_size(h_size,v_size,*fp_bmp_in_1);

	/* 入力画像２の取得と画像サイズのチェック */
	ret=get_bmp_image_size(&h,&v,*fp_bmp_in_2);
	if((h!=*h_size)||(v!=*v_size)) {
		fprintf(stderr,"image sizes mismatched:\n");
		fprintf(stderr,"\tin1: %d x %d\n", *h_size, *v_size);
		fprintf(stderr,"\tin2: %d x %d\n", h, v);

		fclose(*fp_bmp_in_1);
		fclose(*fp_bmp_in_2);
		fclose(*fp_bmp_in_3);
		fclose(*fp_histogram);
		fclose(*fp_bmp_actual);
		fclose(*fp_bmp_ideal);

		exit(1);
	}

	/* 入力画像３の取得と画像サイズのチェック */
	ret=get_bmp_image_size(&h,&v,*fp_bmp_in_3);
	if((h!=*h_size)||(v!=*v_size)) {
		fprintf(stderr,"image sizes mismatched:\n");
		fprintf(stderr,"\tin1: %d x %d\n", *h_size, *v_size);
		fprintf(stderr,"\tin2: %d x %d\n", *h_size, *v_size);
		fprintf(stderr,"\tin3: %d x %d\n", h, v);

		fclose(*fp_bmp_in_1);
		fclose(*fp_bmp_in_2);
		fclose(*fp_bmp_in_3);
		fclose(*fp_histogram);
		fclose(*fp_bmp_actual);
		fclose(*fp_bmp_ideal);

		exit(1);
	}

	/* 正解オブジェクト領域の取得と画像サイズのチェック */
	if (fp_ideal_check) {
		ret=get_bmp_image_size(&h,&v,*fp_bmp_ideal);
		if((h!=*h_size)||(v!=*v_size)) {
			fprintf(stderr,"image sizes mismatched:\n");
			fprintf(stderr,"\tin1: %d x %d\n", *h_size, *v_size);
			fprintf(stderr,"\tin2: %d x %d\n", *h_size, *v_size);
			fprintf(stderr,"\tin3: %d x %d\n", *h_size, *v_size);
			fprintf(stderr,"\tideal_mask: %d x %d\n", h, v);

			fclose(*fp_bmp_in_1);
			fclose(*fp_bmp_in_2);
			fclose(*fp_bmp_in_3);
			fclose(*fp_histogram);
			fclose(*fp_bmp_actual);
			fclose(*fp_bmp_ideal);

			exit(1);
		}
	}
	if (fp_base_check) {
		ret=get_bmp_image_size(&h,&v,*fp_bmp_base);
		if((h!=*h_size)||(v!=*v_size)) {
			fprintf(stderr,"image sizes mismatched:\n");
			fprintf(stderr,"\tin1: %d x %d\n", *h_size, *v_size);
			fprintf(stderr,"\tin2: %d x %d\n", *h_size, *v_size);
			fprintf(stderr,"\tin3: %d x %d\n", *h_size, *v_size);
			fprintf(stderr,"\tbase: %d x %d\n", h, v);

			fclose(*fp_bmp_in_1);
			fclose(*fp_bmp_in_2);
			fclose(*fp_bmp_in_3);
			fclose(*fp_histogram);
			fclose(*fp_bmp_actual);
			fclose(*fp_bmp_base);
			if(fp_ideal_check) fclose(*fp_bmp_ideal);

			exit(1);
		}
	}

	fprintf(stderr," width x length : %d x %d\n",*h_size,*v_size);

	return;
}


/* 量子化（ヒストグラムのビンの決定） */
int quantize (
	float value		/* in：連続値（MIN～MAX） */
)
{
	int output;
	int RANGE;
	int STEP_SIZE;

	STEP_SIZE = (MAX - MIN + 1) / NUM_OF_BINS;
	output = ((int)value - MIN) / STEP_SIZE;

	return(output);		/* out：整数値（０ ～ NUM_OF_BIN-1） */
}


/*２値化処理 */
void binarize (
	unsigned char *input_arr,			/* in：入力画像の配列（２５６諧調、グレー） */
	unsigned char *output_img[3],		/* out：２値画像の配列（カラー画像, RGB全てに同じ値を出力）*/
	float theta,						/* in：閾値*/
	int h_size,							/* in：画像サイズ（ヨコ） */
	int v_size							/* in：画像サイズ（タテ） */
)
{
	int
		i,j,c,pos;

	for(j=0;j<v_size;j++) {
		for(i=0;i<h_size;i++) {
			pos=j*h_size+i;
			if (input_arr[pos] > theta) {
				for (c=0; c<3; c++) output_img[c][pos] = FORE_GROUND;
			} else {
				for (c=0; c<3; c++) output_img[c][pos] = BACK_GROUND;
			}
		} /* for (i) */
	} /* for (j) */

	return;
}


/* 誤差の評価（不一致ピクセル数／総ピクセル数） */
float eval_err_rate (
	unsigned char *actual_img[],		/* in：評価対象*/
	unsigned char *ideal_img[],			/* in：正解データ*/
	int h_size,							/* in：画像サイズ（ヨコ） */
	int v_size							/* in：画像サイズ（タテ） */
)
{
	int		i, j, pos, num_of_error_pix;
	float	error_rate;

	num_of_error_pix=0;					/* 初期化*/
	for(j=0;j<v_size;j++) {
		for(i=0;i<h_size;i++) {
			pos=j*h_size+i;
			if (actual_img[0][pos]!=ideal_img[0][pos])
				num_of_error_pix++;				/* 誤った画素数のカウント*/
		} /* for (i) */
	} /* for (j) */

	/* 誤差の算出*/
	error_rate = 100.0 * (float) num_of_error_pix / (float) (h_size * v_size);

	return(error_rate);		/* out：誤り率*/
}


