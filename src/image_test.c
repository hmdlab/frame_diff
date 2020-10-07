/* -------------------------------------------------------------------------------------
 *
 *	image_test.c:   電生実験　「フレーム間差分オブジェクト抽出」
 *					サンプルプログラム（１）
 * -------------------------------------------------------------------------------------
 * 【処理の概要】
 *  （１）入力画像（カラー、BMP形式、256諧調）を与えられたファイルから読み込む
 *  （２）読み込んだ画像を、左右反転または輝度反転する（引数で処理の内容を指定）
 *  （３）処理結果を画像ファイルに出力する
 *  （４）処理結果のヒストグラムをテキストファイルに出力する
 *
 * 【使用方法】
 *  image_test \
 *		-in (input_file) \			# 入力する画像ファイルの指定
 *		-out　（output_file） \			# 処理結果の画像の出力先の指定
 *		-histogram　(hist_file） \		# ヒストグラムデータの出力先の指定
 *		[-reflect, -negative]		# 処理内容の指定（reflect：左右反転、negative：輝度反転）
 *
 * ------------------------------------------------------------------------------------- */

#include <string.h>
#include"image_io.h"

#define NUM_OF_BINS 64		/* ヒストグラムのビンの総数 */
#define MAX 255				/* 画像データ（BMP形式）のRGB成分の最大値 */
#define MIN 0				/* 画像データ（BMP形式）のRGB成分の最小値*/


/* -------------------------------
 *
 *	関数の宣言部
 *
 * ------------------------------- */
 /* 引数に関するメッセージの表示 */
void usage();

/* 量子化処理（ヒストグラムのビンの決定）*/
int quantize (
	unsigned char value
);



/* -------------------------------
 *
 *	メイン関数
 *
 * ------------------------------- */
int main(int argc,char *argv[])
{
unsigned char
	*input_image[3],		/* 入力画像データ（RGBカラー、256諧調） */
	*output_image[3];		/* 出力画像データ（RGBカラー、256諧調） */

FILE
	*fpin,					/* 入力画像データの読み込み先のファイルポインタ */
	*fpout,					/* 出力画像データの出力先のファイルポインタ */
	*fphist;				/* ヒストグラムデータの出力先のファイルポインタ */

char
	fn_in[100],				/* 入力画像ファイルのファイル名 */
	fn_out[100],			/* 出力画像ファイルのファイル名 */
	fn_hist[100],			/* ヒストグラムデータ・ファイルのファイル名 */
	*string;				/* 引数（文字列）読み取り用バッファ */

int
	h_size,					/* 画像データのサイズ（ヨコ） */
	v_size,					/* 画像データのサイズ（タテ） */
	i,j,					/* 画像平面のX-Y座標 */
	c,						/* 色成分のインデックス（c=0:R, c=1:G, c=2:B） */
	ret,					/* ファイル入出力処理に関するフラグ */
	bin,					/* ヒストグラムのビンの番号 */
	step_size,				/* ヒストグラムの各ビンの幅 */
	fpin_check,				/* 入力画像の読み込み先の指定に関するフラグ */
	fpout_check,			/* 出力画像の出力先の指定に関するフラグ */
	fphist_check,			/* ヒストグラムデータの出力先の指定に関するフラグ */
	reflect_img,			/* 左右反転処理に関するフラグ（0:非実行, 1:実行）*/
	negative_img,			/* 輝度反転処理に関するフラグ（0:非実行, 1:実行）*/
	histograms[3][NUM_OF_BINS]; /* ヒストグラムデータ */



	/*
	 *  初期化処理
	 */
	fpin_check=0;
	fpout_check=0;
	fphist_check=0;
	reflect_img=0;
	negative_img=0;
	i=1;

	/* 引数の読み取り */
	while(i<argc){
     	if(argv[i][0]=='-'){
        		string = argv[i];
        	if(strcmp( string, "-in" ) == 0 ){
          		fpin_check=1;
          		sprintf(fn_in,"%s",argv[i+1]);
          		i+=2;
        	} else if(strcmp( string, "-out" ) == 0 ){
          		fpout_check=1;
          		sprintf(fn_out,"%s",argv[i+1]);
          		i+=2;
        	} else if(strcmp( string, "-histogram" ) == 0 ){
          		fphist_check=1;
          		sprintf(fn_hist,"%s",argv[i+1]);
          		i+=2;
        	} else if(strcmp( string, "-reflect" ) == 0 ){
          		reflect_img=1;
          		i++;
        	} else if(strcmp( string, "-negative" ) == 0 ){
          		negative_img=1;
          		i++;
        	}else{
          		fprintf(stderr,"unknown option %s \n",string);
          		exit(0);
        	}
	}else{
		fprintf(stderr,"unknown option %s \n",argv[i]);
		exit(0);
		}
	}

	/* 入力画像ファイルのオープン */
	if(fpin_check==0){
		fprintf(stderr," set input file name with -in \n");
			usage(argv[0]);
	}else{
		if((fpin=fopen(fn_in,"rb"))==NULL){
			fprintf(stderr," Can't open input file:%s\n", fn_in);
			exit(1);
		}
		fprintf(stdout," input file: %s\n",fn_in);
	}

	/* 出力画像ファイルのオープン */
	if(reflect_img||negative_img) {
		if(fpout_check==0){
			fprintf(stderr," set output file name with -out\n");
			usage(argv[0]);
		}else{
			if((fpout=fopen(fn_out,"wb"))==NULL){
				fprintf(stderr," Can't open output file:%s\n", fn_out);
				exit(1);
			}
			fprintf(stdout," output file: %s\n",fn_out);
		}
	}

	/* ヒストグラムデータファイルのオープン */
	if(fphist_check==0){
		fprintf(stderr," set histogram data file name with -histogram\n");
		usage(argv[0]);
	}else{
		if((fphist=fopen(fn_hist,"wb"))==NULL){
			fprintf(stderr," Can't open histogram data file:%s\n", fn_hist);
			exit(1);
		}
		fprintf(stdout," histogram data file: %s\n",fn_hist);
	}


	/* 画像サイズ（タテ・ヨコ）の取得 */
	ret=get_bmp_image_size(
		&h_size,			/* out:画像サイズ（ヨコ） */
		&v_size,			/* out:画像サイズ（タテ） */
		fpin);				/* in:画像ファイルのポインタ */
	fprintf(stdout," width x length : %d x %d\n",h_size,v_size);

	/* メモリ領域の確保 */
	for(c=0;c<3;c++) input_image[c]=(unsigned char *) malloc(h_size*v_size*sizeof(unsigned char));
	for(c=0;c<3;c++) output_image[c]=(unsigned char *) malloc(h_size*v_size*sizeof(unsigned char));

	/* ヒストグラムデータ（各色・ビンの頻度）の初期化 */
	for (bin=0;bin<NUM_OF_BINS;bin++) {
		for (c=0;c<3;c++ ) {
			histograms[c][bin]=0;
		}
	}



	/*
	 *  画像データの読み込み
	 */
	ret=image_input_bmp(
		input_image,			/* out:画像データ */
		h_size,					/* in:画像サイズ（ヨコ） */
		v_size,					/* in:画像サイズ（タテ） */
		fpin);					/* in:画像ファイルのポインタ */



	/*
	 *  画像の操作（左右反転 or 輝度反転） と ヒストグラムデータの取得
	 */
	for(c=0;c<3;c++){
		for(j=0;j<v_size;j++){
			for(i=0;i<h_size;i++){

				if (reflect_img) {
				/* 左右反転操作（'-reflect'オプション指定時） */
					output_image[c][j*h_size+i]=input_image[c][j*h_size+(h_size-i-1)];
					bin=quantize(output_image[c][j*h_size+i]);
				} else if (negative_img) {
				/* 輝度反転操作（'-negative'オプション指定時） */
					output_image[c][j*h_size+i]=255-input_image[c][j*h_size+i];
					bin=quantize(output_image[c][j*h_size+i]);
				} else {
				/* 画像操作なし（デフォルト） */
					bin=quantize(input_image[c][j*h_size+i]);
				}

				/* 操作後の画像のヒストグラムデータ */
				histograms[c][bin]+=1;
			}
		}
	}


	/*
	 *   処理結果（画像、ヒストグラムデータ）の出力
	 */
	/* 処理結果の画像の出力 */
	if (reflect_img||negative_img)
		ret=image_output_bmp(
				output_image,		/* in:保存する画像データ */
				h_size,				/* in: 画像サイズ（ヨコ）*/
				v_size,				/* in: 画像サイズ（タテ）*/
				fpout);				/* out:保存先のファイルポインタ */

	/* ヒストグラムデータの出力 */
	step_size = (MAX - MIN + 1) / NUM_OF_BINS;  /* 各ビンの幅 */
	fprintf(fphist, "# MIN(bin)\tR[bin]\tG[bin]\tB[bin]\n");	/* 凡例 */
	for (bin=0;bin<NUM_OF_BINS;bin++) {
		fprintf(
			fphist,						/* out:ヒストグラムデータの出力先 */
			"%d\t%d\t%d\t%d\n",			/* in:データファイルの書式 */
			bin*step_size+step_size/2,  /* in:代表値（中央値） */
			histograms[0][bin],			/* in:R成分の値（0～255）の出現頻度 */
			histograms[1][bin],			/* in:G成分の値（0～255）の出現頻度 */
			histograms[2][bin]			/* in:B成分の値（0～255）の出現頻度 */
		);
	}


	/*
	 *  終了処理
	 */
	/* ファイルのクローズ */
	fclose(fpin);
	if(reflect_img||negative_img) fclose(fpout);
	fclose(fphist);

	/* メモリ領域の解放 */
	for(c=0;c<3;c++) free((unsigned char *) input_image[c]);
	if(reflect_img||negative_img) {
		for(c=0;c<3;c++)
			free((unsigned char *) output_image[c]);
	}

	/* 処理終了の通知 */
	fprintf(stdout,"program terminated normally\n");

	return 0;
}



/* -------------------------------
 *
 *	サブ関数
 *
 * ------------------------------- */

/* 引数に関するメッセージの表示 */
void usage(char *s)
{
  fprintf(stdout,
    " image_test -in (input file name) -out (output file name)\n");

  exit(1);
}

/* 量子化（ヒストグラムのビンの決定） */
int quantize (
    unsigned char value	/* in：連続値（MIN～MAX） */
)
{
	int output;
	int RANGE;
	int STEP_SIZE;

	STEP_SIZE = (MAX - MIN + 1) / NUM_OF_BINS;
	output = ((int)value - MIN) / STEP_SIZE;

	return(output); 	/* out：整数値（０ ～ NUM_OF_BIN-1） */
}


