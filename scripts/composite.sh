#!/bin/bash
# ${0} の dirname を取得
cwd=`dirname "${0}"`

# ${0} が 相対パスの場合は cd して pwd を取得
expr "${0}" : "/.*" > /dev/null || cwd=`(cd "${cwd}" && pwd)`

cd "${cwd}/.."

IN1="images/1.bmp"
IN2="images/2.bmp"
IN3="images/3.bmp"
BASE="images/base.bmp"
THETA=10

ALPHA1="out/alpha1vs2.bmp"
ALPHA2="out/alpha2vs3.bmp"

OUT_HIST="out/hist.tsv"
OUT_COMPOSITE="out/composite.bmp"
OUT_MASK="out/mask.bmp"

bin/frame_diff -in1 $IN1 \
               -in2 $IN2 \
               -in3 $IN3 \
               -histogram $OUT_HIST \
               -alpha1 $ALPHA1 \
               -alpha2 $ALPHA2 \
               -theta $THETA \
               -out $OUT_MASK \

# バイナリを実行する
bin/frame_diff -in1 $IN1 \
               -in2 $IN2 \
               -in3 $IN3 \
               -histogram $OUT_HIST \
               -alpha1 $ALPHA1 \
               -alpha2 $ALPHA2 \
               -theta $THETA \
               -out $OUT_COMPOSITE \
               -base $BASE
