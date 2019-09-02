#!/bin/bash

inst=$(apt -qq list docker.io | grep "installed")
if [ ! -n "$inst" ] ; then
	#sudo apt-get remove docker docker-engine docker.io
	sudo apt install docker.io
	sudo adduser $USER docker
	echo "User $USER was added to group docker. You have to log out completely and log in again. Then start this script again!"
	exit 0
	# or
	#sudo snap install docker
fi

docker build --build-arg user=${USER} --build-arg uid=$(id -u) --build-arg gid=$(id -g) -t esp-idf-arduino-esp32:v1 .
