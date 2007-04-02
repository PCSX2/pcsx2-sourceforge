
CC = gcc

PLUGIN = libCDVDiso.so
CFG = cfgCDVDiso
MKISO = mkiso
CFLAGS = -fPIC -Wall -O2 -fomit-frame-pointer -I.. -I. -D__LINUX__
OBJS = ../CDVDiso.o Config.o Linux.o ../libiso.o
CFGOBJS = conf.o interface.o support.o ${OBJS}
LIBS = -lz -lbz2 -lstdc++
CFGLIBS = $(shell pkg-config --libs gtk+-2.0) ${LIBS}
CFLAGS += $(shell pkg-config --cflags gtk+-2.0)
DEPS:= $(OBJS:.o=.d) $(CFGOBJS:.o=.d) 

all: plugin cfg
install: all

plugin: ${OBJS}
	rm -f ${PLUGIN}
	${CC} -shared -Wl,-soname,${PLUGIN} ${CFLAGS} ${OBJS} -o ${PLUGIN} ${LIBS}
	strip --strip-unneeded --strip-debug ${PLUGIN}

cfg: ${CFGOBJS}
	rm -f ${CFG}
	${CC} ${CFLAGS} ${CFGOBJS} -o ${CFG} ${CFGLIBS}
	strip ${CFG}

clean: 
	rm -f ${OBJS} ${DEPS} ${PLUGIN} ${CFGOBJS} ${CFG}

%.o: %.c
	${CC} ${CFLAGS} -c -o $@ $< -MD -MF $(patsubst %.o,%.d,$@)

-include ${DEPS}
