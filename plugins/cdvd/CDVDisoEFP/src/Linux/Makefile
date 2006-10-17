
PLUGIN = libCDVDisoEFP.so
PLUGINOBJS = CDVDiso.o
PLUGINHEADERS = CDVDiso.h
PLUGINFLAGS = -fPIC -Wall -O2 -fomit-frame-pointer -D_LARGEFILE64_SOURCE \
	-I.. -I. -I./Linux
PLUGINLIBS =

CFG = cfgCDVDisoEFP
CFGOBJS = interface.o aboutbox.o mainbox.o selectionbox.o \
	devicebox.o conversionbox.o progressbox.o messagebox.o \
	tablerebuild.o device.o CD.o DVD.o
CFGHEADERS = interface.h aboutbox.h mainbox.h selectionbox.h \
	devicebox.h conversionbox.h progressbox.h messagebox.h \
	tablerebuild.h device.h CD.h DVD.h
CFGFLAGS = -fPIC -Wall -O2 -fomit-frame-pointer -D_LARGEFILE64_SOURCE \
	-I.. -I. -I./Linux
CFGLIBS =

COMP = compCDVDisoEFP
COMPOBJS = comparisonbox.o progressbox.o messagebox.o \
	comparisondummy.o
COMPHEADERS = progressbox.h messagebox.h

SHAREDOBJS = ../version.o actualfile.o ../isofile.o conf.o logfile.o \
	../imagetype.o ../multifile.o ../isocompress.o ../convert.o \
	../gzipv1.o ../blockv2.o ../gzipv2.o ../bzip2v2.o  ../ecma119.o \
	../toc.o ../ini.o ../bzip2v3.o \
	../zlib/adler32.o ../zlib/compress.o ../zlib/crc32.o ../zlib/gzio.o \
	../zlib/uncompr.o ../zlib/deflate.o ../zlib/trees.o ../zlib/zutil.o \
	../zlib/inflate.o ../zlib/infback.o ../zlib/inftrees.o \
	../zlib/inffast.o \
	../bzip2/blocksort.o ../bzip2/bzlib.o ../bzip2/compress.o \
	../bzip2/crctable.o ../bzip2/decompress.o ../bzip2/huffman.o \
	../bzip2/randtable.o
SHAREDHEADERS = ../version.h actualfile.h ../isofile.h conf.h logfile.h \
	../imagetype.h ../multifile.h ../isocompress.h ../convert.h \
	../gzipv1.h ../blockv2.h ../gzipv2.h ../bzip2v2.h ../ecma119.h \
	../toc.h ../ini.h ../bzip2v3.h


CC = gcc

GTKFLAGS = $(shell pkg-config --cflags gtk+-2.0)

//GTKFLAGS += -DG_DISABLE_DEPRECATED \
//	-DGDK_DISABLE_DEPRECATED \
//	-DGDK_PIXBUF_DISABLE_DEPRECATED \
//	-DGTK_DISABLE_DEPRECATED

GTKLIBS = $(shell pkg-config --libs gtk+-2.0)
# Do we need to remove "-rdynamic" as well? Or is that just the main program?


all: plugin cfg

release: plugin cfg
	cp $(PLUGIN) ../..
	cp $(CFG) ../..

plugin: $(PLUGINOBJS) $(SHAREDOBJS)
	rm -f $(PLUGIN)
	$(CC) -shared -Wl,-soname,$(PLUGIN) $(PLUGINFLAGS) $(PLUGINLIBS) \
	$(PLUGINOBJS) $(SHAREDOBJS) -o $(PLUGIN)
	strip --strip-unneeded --strip-debug $(PLUGIN)

cfg: $(CFGOBJS) $(SHAREDOBJS)
	rm -f $(CFG)
	$(CC) $(CFGFLAGS) $(GTKFLAGS) $(CFGLIBS) $(GTKLIBS) \
	$(CFGOBJS) $(SHAREDOBJS) -o $(CFG)
	strip $(CFG)

compare: $(COMPOBJS) $(SHAREDOBJS)
	rm -f $(COMP)
	$(CC) $(CFGFLAGS) $(GTKFLAGS) $(CFGLIBS) $(GTKLIBS) \
	$(COMPOBJS) $(SHAREDOBJS) -o $(COMP)
	strip $(COMP)

$(PLUGINOBJS) $(SHAREDOBJS): %.o: %.c
	$(CC) $(PLUGINFLAGS) -c $< -o $@

$(CFGOBJS) ../comparisonbox.o ../comparisondummy.o: %.o: %.c
	$(CC) $(CFGFLAGS) $(GTKFLAGS) -c $< -o $@

.PHONY : clean allclean
clean:
	-rm -f $(PLUGINOBJS) $(PLUGIN) $(CFGOBJS) $(CFG) \
	$(COMP) $(COMPOBJS) $(SHAREDOBJS)
	-rm -f *~ temp.txt ../*~ ../temp.txt ../../*~

allclean:
	-rm -f $(PLUGINOBJS) $(PLUGIN) $(CFGOBJS) $(CFG) \
	$(COMP) $(COMPOBJS) $(SHAREDOBJS)
	-rm -f *~ temp.txt ../*~ ../temp.txt ../../*~
	-rm -f ../../$(PLUGIN) ../../$(CFG)

