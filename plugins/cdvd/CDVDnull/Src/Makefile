#
# Makefile for MINGW32
#


all: cdvdnull

PLUGIN = CDVDnull.dll

CC = gcc
NASM = nasmw
RM = rm -f
AR = ar
STRIP = strip
RC = windres

OPTIMIZE = -O2 -fomit-frame-pointer -finline-functions -ffast-math -fno-strict-aliasing
FLAGS = -D__WIN32__ -D__MINGW32__ # -DENABLE_NLS -DPACKAGE=\"pcsx2\"
RC1FLAGS = -d__MINGW32__
LIBS = -L./ -lcomctl32 -lwinmm -lgdi32 -lcomdlg32 #-lintl -lwsock32 
RESOBJ = cdvdnull.o

OBJS = CDVD.o


DEPS:= $(OBJS:.o=.d)

CFLAGS = -Wall ${OPTIMIZE} -I. -I/usr/local/include ${FLAGS}

cdvdnull: ${OBJS}
	dllwrap --def plugin.def -o ${PLUGIN} ${OBJS} ${LIBS}
#	${CC} -shared -Wl,--kill-at,--output-def,plugin.def ${CFLAGS} ${OBJS} -o ${PLUGIN} ${LIBS}
	${STRIP} ${PLUGIN}

.PHONY: clean cdvdnull

clean:
	${RM} ${OBJS} ${DEPS} ${PCSX2}

%.o: %.asm
	${NASM} ${ASMFLAGS} -o $@ $<

%.o: %.c
	${CC} ${CFLAGS} -c -o $@ $< -MD -MF $(patsubst %.o,%.d,$@)

${RESOBJ}: CDVDnull.rc
	    ${RC} -D__MINGW32__ -I rc -O coff -o $@ -i $<

-include ${DEPS}

	