
COMPONENT_ADD_INCLUDEDIRS := ../rem/include

COMPONENT_SRCDIRS = \
	../rem/src/fir \
	../rem/src/au \
	../rem/src/goertzel \
	../rem/src/dtmf \
	../rem/src/auresamp \
	../rem/src/aac \
	../rem/src/aumix \
	../rem/src/g711 \
	../rem/src/auconv \
	../rem/src/aubuf \
	../rem/src/aufile \
	../rem/src/autone \
	../rem/src/vid \
	../rem/src/vidconv \

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

COMPONENT_OBJEXCLUDE = 
