#!/bin/bash
spath=`pwd`

export IDF_PATH=$spath/esp-idf ; export PATH=$PATH:$spath/crosstool-NG/builds/xtensa-esp32-elf:$spath/xtensa-esp32-elf/bin:$spath/crosstool-NG/builds/xtensa-esp32-elf:$spath/xtensa-esp32-elf/bin
