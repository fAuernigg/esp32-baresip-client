
COMPONENT_ADD_INCLUDEDIRS := ../baresip/include

COMPONENT_SRCDIRS = \
../baresip/src \
../baresip/modules/g711 \
../baresip/modules/i2s \
../baresip/modules/stdio \
../baresip/modules/menu \
.

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
CFLAGS	+= -Wno-char-subscripts

CFLAGS	+= -DSHARE_PATH=\"/usr/share/baresip\"
CFLAGS	+= -Wno-error=address

CFLAGS    += -DSTATIC=1 -DEXTCONFIG=1 -DNODNS
CXXFLAGS  += -DSTATIC=1 -DEXTCONFIG=1 -DNODNS

COMPONENT_OBJEXCLUDE = \
