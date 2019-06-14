#!/bin/bash 
cd `dirname $0` 
spath=`pwd`

sudo apt-get install -y git wget make libncurses-dev flex bison gperf python python-serial gawk gperf grep gettext libncurses-dev python python-dev automake bison flex texinfo help2man libtool
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

if [ ! -d "crosstool-NG" ] ; then
	git clone -b xtensa-1.22.x https://github.com/espressif/crosstool-NG.git
	if [ $? -ne 0 ] ; then echo "error git clone crosstool-NG" ; exit 1; fi

	cd crosstool-NG

	# fix file download problem:
	mkdir -p .build/tarballs
	cp ../expat-2.1.0.tar.gz .build/tarballs/expat-2.1.0.tar.gz

	if [ $? -ne 0 ] ; then echo "error cd crosstool-NG" ; exit 1; fi
	./bootstrap
	if [ $? -ne 0 ] ; then echo "error ./bootstrap" ; exit 1; fi


	./configure --prefix=$PWD
	if [ $? -ne 0 ] ; then echo "error ./configure --prefix=$PWD ctg" ; exit 1; fi

	make install
	if [ $? -ne 0 ] ; then echo "error ./ct-ng make install" ; exit 1; fi

	./ct-ng xtensa-esp32-elf
	if [ $? -ne 0 ] ; then echo "error ./ct-ng xtensa-esp32-elf" ; exit 1; fi

	./ct-ng build
	if [ $? -ne 0 ] ; then echo "error ./ct-ng build" ; exit 1; fi

	chmod -R u+w builds/xtensa-esp32-elf
	if [ $? -ne 0 ] ; then echo "error chmod -R u+w builds/xtensa-esp32-elf" ; exit 1; fi
fi

cd $spath
if [ $? -ne 0 ] ; then echo "error cd ~/$spath" ; exit 1; fi

if [ ! -d "esp-idf" ] ; then
	git clone --recursive https://github.com/espressif/esp-idf.git
	if [ $? -ne 0 ] ; then echo "error  git clone esp-idf.git" ; exit 1; fi
	git checkout release/v3.2
else
	cd esp-idf
	git pull
	git checkout release/v3.2
	cd ..
fi

cd esp-idf
if [ $? -ne 0 ] ; then echo "error cd esp-idf" ; exit 1; fi

git submodule update --init
if [ $? -ne 0 ] ; then echo "error git submodule update --init esp-idf failed" ; exit 1; fi


cd $spath/newproject/
if [ ! -d "components" ] ; then
	mkdir components
fi
cd components

if [ ! -d "arduino" ] ; then
	# install arduino-esp32 as component of esp-idf
	git clone https://github.com/espressif/arduino-esp32.git arduino
	if [ $? -ne 0 ] ; then echo "error cloning arduino-esp32" ; exit 11 ; fi
fi

cd arduino
if [ $? -ne 0 ] ; then echo "error changing into arduino compoment" ; exit 11 ; fi
git submodule update --init --recursive
if [ $? -ne 0 ] ; then echo "error update arduino-esp32 submodules" ; exit 11 ; fi


cd $spath

echo "----------------------"
echo "installtoolchain done"
echo "----------------------"
echo " "
echo "--------------------------------------------------------------"
echo "source the toolchain by running . ./initpath.sh"
echo "--------------------------------------------------------------"

