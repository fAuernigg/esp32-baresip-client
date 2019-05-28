#!/bin/bash
cd `dirname $0`
spath=`pwd`

export IDF_PATH=$spath/esp-idf ; export PATH=$PATH:$spath/crosstool-NG/builds/xtensa-esp32-elf:$spath/xtensa-esp32-elf/bin:$spath/crosstool-NG/builds/xtensa-esp32-elf:$spath/xtensa-esp32-elf/bin
