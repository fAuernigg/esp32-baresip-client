#!/bin/bash

cd `dirname $0`
spath=`pwd`
cd ..

export IDF_PATH=`pwd`/esp-idf
export PATH=$PATH:`pwd`/crosstool-NG/builds/xtensa-esp32-elf:`pwd`/xtensa-esp32-elf/bin:`pwd`/crosstool-NG/builds/xtensa-esp32-elf:`pwd`/xtensa-esp32-elf/bin

cd $spath
make monitor 2>&1
