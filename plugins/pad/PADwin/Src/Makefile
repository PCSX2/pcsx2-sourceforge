
PLUGIN  = libPADwin.so
CFLAGS  = -fPIC -Wall -O2 -fomit-frame-pointer -D__LINUX__
OBJS    = PAD.o Conf.o Linux.o #SDL.o
OBJS   += interface.o support.o
LIBS    = $(shell gtk-config --libs) $(shell sdl-config --libs)
CFLAGS += $(shell gtk-config --cflags) $(shell sdl-config --cflags)

DEPS:= $(OBJS:.o=.d)

all: plugin

plugin: ${OBJS}
	rm -f ${PLUGIN}
	gcc -shared -Wl,-soname,${PLUGIN} ${CFLAGS} ${OBJS} -o ${PLUGIN} ${LIBS}
	strip --strip-unneeded --strip-debug ${PLUGIN}

.PHONY: clear plugin

clean: 
	rm -f ${OBJS} ${DEPS} ${PLUGIN}

-include ${DEPS}

