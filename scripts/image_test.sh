#!/bin/bash
# ${0} の dirname を取得
cwd=`dirname "${0}"`

# ${0} が 相対パスの場合は cd して pwd を取得
expr "${0}" : "/.*" > /dev/null || cwd=`(cd "${cwd}" && pwd)`

cd "${cwd}/.."

# バイナリを実行する
bin/image_test -in images/flower.bmp \
               -histogram out/flower_hist.csv \

bin/image_test -in images/flower.bmp \
               -out out/flower_reflect.bmp \
               -histogram out/flower_refl_hist.csv \
               -reflect
               
bin/image_test -in images/flower.bmp \
               -out out/flower_invert.bmp \
               -histogram out/flower_inv_hist.csv \
               -negative