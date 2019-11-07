FROM ubuntu:18.04

RUN apt-get update
RUN apt-get -y install libcups2- aptitude git wget make libncurses-dev flex bison gperf python python-serial gawk gperf grep gettext libncurses-dev python python-dev automake bison flex texinfo help2man libtool python-pip
# Note: libcups2 archive can not found download server
#       Therefore it is disabled in apt install cmd using minus char at the end "libcups2-"

ADD scripts /home/scripts

ARG user
ARG uid
ARG gid
WORKDIR /home/$user
RUN echo "USER=$user"
RUN groupadd --gid $gid $user
RUN useradd -m -u $gid -g $gid -p jackson -s /bin/bash $user
RUN chown $user:$user /home/$user
RUN chown $user:$user /home/
USER $user

RUN touch .bashrc

RUN compilertar="xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz" ; wget "https://dl.espressif.com/dl/$compilertar" ; tar -xzf "$compilertar" ; rm $compilertar

ADD esp-idf-requirements.txt esp-idf-requirements.txt
RUN python -m pip install --user -r esp-idf-requirements.txt
