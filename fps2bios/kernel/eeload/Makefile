#  _____     ___ ____
#   ____|   |    ____|      PSX2 OpenSource Project
#  |     ___|   |____       (C)2002, David Ryan ( Oobles@hotmail.com )
#  ------------------------------------------------------------------------

# Generated automatically from Makefile.in by configure.
#.SUFFIXES: .S .c .o .s .elf .irx

#  ------------------------------------------------------------------------
#  COMPILERS

IOPCC = iop-gcc
IOPAR = iop-ar
IOPLD = iop-ld
IOPAS = iop-as
EECC  = ee-gcc
EEAR  = ee-ar
EELD  = ee-gcc


#  ------------------------------------------------------------------------
#  DIRECTORY PATHS & FLAGS


EECFLAGS  = -O2 -fomit-frame-pointer -mips3 -EL -nostartfiles -G0 -D_EE --save-temps
EEINCLUDES = -I. -Iinclude -I$(PS2LIB)/common/include -I$(PS2LIB)/ee/include

IOPCFLAGS = -O2 -fomit-frame-pointer -nostartfiles 
IOPINCLUDES = -I. -Iinclude -I$(PS2LIB)/common/include -I$(PS2LIB)/iop/include
IOPCOMPILE = $(IOPCC) $(IOPINCLUDES) $(IOPCFLAGS)
IOPLINK = $(IOPLD) -dc


#  ------------------------------------------------------------------------
#  PROJECTS TO BUILD

all: eeload


#  ------------------------------------------------------------------------
#  KERNEL BUILD INSTRUCTIONS

LDFLAGS = -L$(PS2LIB)/ee/lib
LDADD   = -lmc -lpad -lc -lkernel
OBJECTS = eedata.o eeirq.o eeload.o eeinit.o eeelf.o eedebug.o romdir.o

eeload: $(OBJECTS)
	$(EELD) -Wl,--oformat,binary,--Map,eeload.map -T linkfile $(EECFLAGS) $(OBJECTS) $(LDFLAGS) $(LDADD) -o ../../build/EELOAD

%.o: %.c
	$(EECC) $(EEINCLUDES) $(EECFLAGS) -o $@ -c $<


clean:
	rm -f $(OBJECTS) eeload



