
CC = gcc

PLUGIN = libDEV9null.so
CFLAGS+= -fPIC -Wall -O2 -fomit-frame-pointer -D__LINUX__
OBJS = dev9null.o
DEPS:= $(OBJS:.o=.d)
LIBS = $(shell gtk-config --libs) 
CFLAGS+= $(shell gtk-config --cflags)

all: plugin

plugin: ${OBJS}
	rm -f ${PLUGIN}
	gcc -shared -Wl,-soname,${PLUGIN} ${CFLAGS} ${OBJS} -o ${PLUGIN} ${LIBS}
	strip --strip-unneeded --strip-debug ${PLUGIN}

clean: 
	rm -f ${OBJS} ${DEPS}

%.o: %.c
	${CC} ${CFLAGS} -c -o $@ $< -MD -MF $(patsubst %.o,%.d,$@)

-include ${DEPS}
