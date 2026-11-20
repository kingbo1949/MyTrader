#!/bin/zsh

ICE_BIN=/Users/fantang/local/ice.3.7.9/debug/bin
ICE_SLICE=/Users/fantang/Downloads/ice-3.7.9/slice

# slice2cpp把.h和.cpp都写到同一个--output-dir，但本项目要求.h在include/、.cpp在src/，
# 所以先生成到临时目录，再按后缀分流过去。
# 不加--impl-c++11：*I.cpp/*I.h是当年生成的骨架、现在已经是(哪怕只是占位的)落地代码，
# 每次都重新生成骨架只会跟已存在的文件冲突报错("already exists - will not overwrite")，
# 而且也没有必要——真正要刷新的只是QStruc/QDatabase本体的绑定代码。

TMP_DIR=$(mktemp -d)

$ICE_BIN/slice2cpp -I. -I$ICE_SLICE --underscore --output-dir $TMP_DIR QStruc.ice
$ICE_BIN/slice2cpp -I. -I$ICE_SLICE --underscore --output-dir $TMP_DIR QDatabase.ice

mv $TMP_DIR/*.h ./include/
mv $TMP_DIR/*.cpp ./src/
rmdir $TMP_DIR

echo "Done!"
