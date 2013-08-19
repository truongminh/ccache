#############################################################################
# Makefile for building: ccache
# Project:  ../ccache.pro
# Template: app
#############################################################################

####### Compiler, tools and options

CC            = gcc
CXX           = g++
DEFINES       = 
CFLAGS        = -m64 -pipe -O2 -g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fstack-protector --param=ssp-buffer-size=4 -m64 -mtune=generic -Wall -W $(DEFINES)
CXXFLAGS      = -m64 -pipe -O2 -g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fstack-protector --param=ssp-buffer-size=4 -m64 -mtune=generic -Wall -W $(DEFINES)
INCPATH       = -I.. -I../src -I/usr/local/include/opencv -I.. -I.
LINK          = g++
LFLAGS        = -m64 -Wl,-O1
LIBS          = $(SUBLIBS)   -L/usr/local/lib/ -lopencv_core -lopencv_highgui -lopencv_imgproc -L/usr/lib/ -lpthread -lopencv_legacy 
AR            = ar cqs
RANLIB        = 
TAR           = tar -cf
COMPRESS      = gzip -9f
COPY          = cp -f
SED           = sed
COPY_FILE     = $(COPY)
COPY_DIR      = $(COPY) -r
STRIP         = 
INSTALL_FILE  = install -m 644 -p
INSTALL_DIR   = $(COPY_DIR)
INSTALL_PROGRAM = install -m 755 -p
DEL_FILE      = rm -f
SYMLINK       = ln -f -s
DEL_DIR       = rmdir
MOVE          = mv -f
CHK_DIR_EXISTS= test -d
MKDIR         = mkdir -p

####### Output directory

OBJECTS_DIR   = ./

####### Files

SOURCES       = ../src/main.c \
                ../src/cache/mcache.c \
                ../src/cache/cache.c \
		../src/http/request_handler.c \
                ../src/http/request.c \
                ../src/http/reply.c \
                ../src/lib/util.c \
                ../src/lib/sds.c \
                ../src/lib/safe_queue.c \
                ../src/lib/objSds.c \
                ../src/lib/dicttype.c \
                ../src/lib/dict.c \
                ../src/lib/adlist.c \
                ../src/lib/ufile.c \
                ../src/lib/mhash.c \
                ../src/net/client.c \
                ../src/net/anet.c \
                ../src/net/ae.c \
                ../src/organizer/bio.c \
                ../src/service/zoom.c \
                ../src/net/http_server.c
OBJECTS       = main.o \
		mcache.o \
		cache.o \
		request_handler.o \
		request.o \
		reply.o \
		util.o \
		sds.o \
		safe_queue.o \
		objSds.o \
		dicttype.o \
		dict.o \
		adlist.o \
		ufile.o \
		mhash.o \
		client.o \
		anet.o \
		ae.o \
		bio.o \
		zoom.o \
		http_server.o
QMAKE_TARGET  = ccache
DESTDIR       = 
TARGET        = ccache

first: all
####### Implicit rules

.SUFFIXES: .o .c .cpp .cc .cxx .C

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.cc.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.cxx.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.C.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.c.o:
	$(CC) -c $(CFLAGS) $(INCPATH) -o "$@" "$<"

####### Build rules

all: Makefile $(TARGET)

$(TARGET):  $(OBJECTS)  
	$(LINK) $(LFLAGS) -o $(TARGET) $(OBJECTS) $(OBJCOMP) $(LIBS)

dist: 
        @$(CHK_DIR_EXISTS) ../.tmp/ccache1.0.0 || $(MKDIR) ../.tmp/ccache1.0.0
	$(COPY_FILE) --parents $(SOURCES) $(DIST) ../.t..1.0.0/ && (cd `dirname ../.t..1.0.0` && $(TAR) ccache1.0.0.tar ccache1.0.0 && $(COMPRESS) ccache1.0.0.tar) && $(MOVE) `dirname ../.t..1.0...1.0.0.tar.gz . && $(DEL_FILE) -r ../.t..1.0.0


clean:compiler_clean 
	-$(DEL_FILE) $(OBJECTS)
	-$(DEL_FILE) *~ core *.core


####### Sub-libraries

distclean: clean
	-$(DEL_FILE) $(TARGET) 
	-$(DEL_FILE) Makefile


compiler_rcc_make_all:
compiler_rcc_clean:
compiler_uic_make_all:
compiler_uic_clean:
compiler_yacc_decl_make_all:
compiler_yacc_decl_clean:
compiler_yacc_impl_make_all:
compiler_yacc_impl_clean:
compiler_lex_make_all:
compiler_lex_clean:
compiler_clean: 

####### Compile

main.o: ../src/main.c ../src/ccache_config.h \
                ../src/organizer/bio.h \
                ../src/cache/mcache.h \
                ../src/signal_handler.h \
                ../src/http/request_handler.h \
                ../src/http/request.h \
                ../src/http/reply.h \
                ../src/usage.c \
                ../src/net/http_server.h \
                ../src/net/ae.h
        $(CC) -c $(CFLAGS) $(INCPATH) -o main.o ../src/main.c

mcache.o: ../src/cache/mcache.c ../src/cache/mcache.h \
                ../src/organizer/bio.h \
                ../src/ccache_config.h \
                ../src/cache/cache.h
        $(CC) -c $(CFLAGS) $(INCPATH) -o mcache.o ../src/cache/mcache.c

cache.o: ../src/cache/cache.c ../src/cache/cache.h \
                ../src/ccache_config.h
        $(CC) -c $(CFLAGS) $(INCPATH) -o cache.o ../src/cache/cache.c

request_handler.o: ../src/http/request_handler.c ../src/http/request_handler.h \
                ../src/http/request.h \
                ../src/http/reply.h
        $(CC) -c $(CFLAGS) $(INCPATH) -o request_handler.o ../src/http/request_handler.c

request.o: ../src/http/request.c ../src/http/request.h
        $(CC) -c $(CFLAGS) $(INCPATH) -o request.o ../src/http/request.c

reply.o: ../src/http/reply.c ../src/http/reply.h
        $(CC) -c $(CFLAGS) $(INCPATH) -o reply.o ../src/http/reply.c

util.o: ../src/lib/util.c ../src/ccache_config.h \
                ../src/lib/util.h
        $(CC) -c $(CFLAGS) $(INCPATH) -o util.o ../src/lib/util.c

sds.o: ../src/lib/sds.c ../src/lib/sds.h
        $(CC) -c $(CFLAGS) $(INCPATH) -o sds.o ../src/lib/sds.c

safe_queue.o: ../src/lib/safe_queue.c ../src/lib/safe_queue.h
        $(CC) -c $(CFLAGS) $(INCPATH) -o safe_queue.o ../src/lib/safe_queue.c

objSds.o: ../src/lib/objSds.c ../src/lib/objSds.h \
                ../src/ccache_config.h \
                ../src/lib/sds.h \
                ../src/lib/adlist.h
        $(CC) -c $(CFLAGS) $(INCPATH) -o objSds.o ../src/lib/objSds.c

dicttype.o: ../src/lib/dicttype.c ../src/lib/dicttype.h \
                ../src/lib/dict.h \
                ../src/lib/adlist.h \
                ../src/lib/sds.h \
                ../src/lib/objSds.h \
                ../src/ccache_config.h
        $(CC) -c $(CFLAGS) $(INCPATH) -o dicttype.o ../src/lib/dicttype.c

dict.o: ../src/lib/dict.c ../src/lib/dict.h
        $(CC) -c $(CFLAGS) $(INCPATH) -o dict.o ../src/lib/dict.c

adlist.o: ../src/lib/adlist.c ../src/lib/adlist.h
        $(CC) -c $(CFLAGS) $(INCPATH) -o adlist.o ../src/lib/adlist.c

ufile.o: ../src/lib/ufile.c ../src/lib/ufile.h \
                ../src/lib/sds.h \
                ../src/lib/adlist.h \
                ../src/ccache_config.h
        $(CC) -c $(CFLAGS) $(INCPATH) -o ufile.o ../src/lib/ufile.c

mhash.o: ../src/lib/mhash.c ../src/lib/mhash.h
        $(CC) -c $(CFLAGS) $(INCPATH) -o mhash.o ../src/lib/mhash.c

client.o: ../src/net/client.c ../src/net/client.h \
                ../src/ccache_config.h \
                ../src/net/http_server.h \
                ../src/net/ae.h \
                ../src/net/anet.h \
                ../src/http/request_handler.h \
                ../src/http/request.h \
                ../src/http/reply.h
        $(CC) -c $(CFLAGS) $(INCPATH) -o client.o ../src/net/client.c

anet.o: ../src/net/anet.c ../src/net/anet.h
        $(CC) -c $(CFLAGS) $(INCPATH) -o anet.o ../src/net/anet.c

ae.o: ../src/net/ae.c ../src/net/ae.h \
                ../src/ccache_config.h \
                ../src/net/client.h \
                ../src/net/http_server.h \
                ../src/cache/mcache.h
        $(CC) -c $(CFLAGS) $(INCPATH) -o ae.o ../src/net/ae.c

bio.o: ../src/organizer/bio.c ../src/organizer/bio.h \
                ../src/ccache_config.h
        $(CC) -c $(CFLAGS) $(INCPATH) -o bio.o ../src/organizer/bio.c

zoom.o: ../src/service/zoom.c ../src/organizer/bio.h \
                ../src/ccache_config.h
        $(CC) -c $(CFLAGS) $(INCPATH) -o zoom.o ../src/service/zoom.c

http_server.o: ../src/net/http_server.c ../src/net/http_server.h \
                ../src/net/ae.h \
                ../src/ccache_config.h \
                ../src/cache/mcache.h \
                ../src/net/client.h
        $(CC) -c $(CFLAGS) $(INCPATH) -o http_server.o ../src/net/http_server.c

####### Install

install:   FORCE

uninstall:   FORCE

FORCE:

