#!/bin/bash 
cd `dirname $0` 
spath=`pwd`

sudo apt-get install -y git wget make libncurses-dev flex bison gperf python python-serial gawk gperf grep gettext libncurses-dev python python-dev automake bison flex texinfo help2man libtool libtool-bin
#if [ $? -ne 0 ] ; then echo "error apt install" ; exit 1; fi


if  [ ! -d "xtensa-esp32-elf" ] ; then
	#file="xtensa-esp32-elf-linux64-1.22.0-61-gab8375a-5.2.0.tar.gz"
	file="xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz"
	wget "https://dl.espressif.com/dl/$file"
	tar -xzf "$file"

	if [ $? -ne 0 ] ; then echo "error untar xtensa-esp32 compiler" ; exit 1; fi

	if  [ ! -d "xtensa-esp32-elf" ] ; then echo "error xtensa-esp32-elf download untar failed" ; exit 1; fi
fi
export PATH=$PATH:$spath/xtensa-esp32-elf/bin


cd $spath
git submodule update --init
if [ $? -ne 0 ] ; then echo "error cd ~/$spath" ; exit 1; fi

cd $spath/esp-idf/
if [ $? -ne 0 ] ; then echo "error cd esp-idf" ; exit 1; fi
git submodule update --init
if [ $? -ne 0 ] ; then echo "error git submodule update --init esp-idf failed" ; exit 1; fi

git checkout release/v3.2

cd "$spath"
python -m pip install --user -r esp-idf/requirements.txt


cd $spath/esp32phone/components/re/
if [ $? -ne 0 ] ; then echo "error components/re does not exist" ; exit 1; fi

#git checkout *
git checkout "esp_v0.5.8"

#patch -p1 -i ../re_mk/patches/0001-fix-build-for-esp32.patch
#patch -p1 -i ../re_mk/patches/0002-tcp-udp-hmac-apis-fix-multi-defs.patch

cd $spath/esp32phone/components/baresip/
if [ $? -ne 0 ] ; then echo "error components/baresip does not exist" ; exit 1; fi

git checkout "esp32_v0.5.10"

cd $spath


echo "----------------------"
echo "installtoolchain done"
echo "----------------------"
echo " "
echo "--------------------------------------------------------------"
echo "source the toolchain by running . ./initpath.sh"
echo "--------------------------------------------------------------"

