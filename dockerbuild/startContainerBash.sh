#!/bin/bash
cd $(dirname $0)

MAPPED_PATHS="
-v $(pwd)/..:$HOME/esp32-project/ 
"

SSH_KEYS="
-v $HOME/.ssh/:$HOME/.ssh/
-v $(dirname $SSH_AUTH_SOCK):$(dirname $SSH_AUTH_SOCK) 
-e SSH_AUTH_SOCK=$SSH_AUTH_SOCK 
"

#DNS="--dns=...."
DNS=

eval `ssh-agent -s`

# add ssh keys before starting bash in container
docker run $MAPPED_PATHS $SSH_KEYS $DNS --user $(id -u):$(id -g) -it esp-idf-arduino-esp32:v1 /home/scripts/mybash.sh "ssh-add $HOME/.ssh/id_rsa" "export IDF_PATH=$HOME/esp32-project/esp-idf" "export PATH=$PATH:$HOME/xtensa-esp32-elf/bin" "/bin/bash"
