
COMPONENT_ADD_INCLUDEDIRS := ../re/include

COMPONENT_SRCDIRS = \
../re/src  \
../re/src/main \
../re/src/sys \
../re/src/dns \
../re/src/mbuf \
../re/src/list \
../re/src/mqueue \
../re/src/mem \
../re/src/tcp \
../re/src/msg \
../re/src/sipevent \
../re/src/udp \
../re/src/tmr \
../re/src/fmt \
../re/src/bfcp \
../re/src/json \
../re/src/aes \
../re/src/dbg \
../re/src/md5 \
../re/src/sa \
../re/src/sdp \
../re/src/hash \
../re/src/httpauth \
../re/src/uri \
../re/src/jbuf \
../re/src/stun \
../re/src/srtp \
../re/src/ice \
../re/src/turn \
../re/src/mod \
../re/src/sip \
../re/src/websock \
../re/src/conf \
../re/src/hmac \
../re/src/rtp \
../re/src/natbd \
../re/src/sipreg \
../re/src/base64 \
../re/src/sipsess \
../re/src/odict \
../re/src/lock \
../re/src/http \
../re/src/net \
../re/src/telev \
../re/src/sha \
../re/src/crc32 \


CFLAGS	+= -DHAVE_SELECT -DHAVE_SELECT_H
CFLAGS	+= -DHAVE_INET_NTOP -DHAVE_INET_PTON -DHAVE_PTHREAD -DHAVE_STRERROR_R
CFLAGS	+= -DHAVE_INTTYPES_H -DHAVE_STDBOOL_H -DHAVE_FORK
CFLAGS	+= -DHAVE_PWD_H
CFLAGS  += -DHAVE_SIGNAL -DHAVE_SYS_TIME_H
CFLAGS  += -DHAVE_UNISTD_H -DHAVE_STRINGS_H
CFLAGS	+= -DHAVE_ROUTE_LIST

CFLAGS	+= -Wall
CFLAGS	+= -Wmissing-declarations
CFLAGS	+= -Wmissing-prototypes
CFLAGS	+= -Wbad-function-cast
CFLAGS	+= -Wsign-compare
CFLAGS	+= -Wnested-externs
CFLAGS	+= -Wshadow
CFLAGS	+= -Waggregate-return
CFLAGS  += -DLINUX -g -std=c99
CFLAGS  += -Os
CFLAGS  += -Wno-error=char-subscripts
CFLAGS 	+= -Wno-error=implicit-function-declaration


COMPONENT_OBJEXCLUDE = \
					   ../re/src/main/openssl.o \
					   ../re/src/main/epoll.o \
					   ../re/src/dns/res.o \
					   ../re/src/mod/dl.o \
					   ../re/src/lock/rwlock.o \
					   ../re/src/net/ifaddrs.o
