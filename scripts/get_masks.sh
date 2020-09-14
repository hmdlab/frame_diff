#!/bin/sh
# ${0} の dirname を取得
cwd=`dirname "${0}"`

# ${0} が 相対パスの場合は cd して pwd を取得
expr "${0}" : "/.*" > /dev/null || cwd=`(cd "${cwd}" && pwd)`

cd "${cwd}/.."

# バイナリを実行する
bin/frame_diff -in1 images/001.bmp \
               -in2 images/002.bmp \
               -in3 images/003.bmp \
               -histogram out/hist.csv \
               -alpha1 out/alpha1vs2.bmp \
               -alpha2 out/alpha2vs3.bmp \
               -theta 10 \
               -out out/mask.bmp \
               -ideal images/ideal.bmp