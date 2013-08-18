#############################################################################
# Makefile for building: ccache
# Project:  ../ccache/ccache.pro
# Template: app
#############################################################################

####### Compiler, tools and options

CC            = gcc
CXX           = g++
DEFINES       = 
CFLAGS        = -m64 -pipe -O2 -g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fstack-protector --param=ssp-buffer-size=4 -m64 -mtune=generic -Wall -W $(DEFINES)
CXXFLAGS      = -m64 -pipe -O2 -g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fstack-protector --param=ssp-buffer-size=4 -m64 -mtune=generic -Wall -W $(DEFINES)
INCPATH       = -I../ccache -I../ccache/src -I/usr/local/include/opencv -I../ccache -I.
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

SOURCES       = ../ccache/src/main.c \
		../ccache/src/cache/mcache.c \
		../ccache/src/cache/cache.c \
		../ccache/src/http/request_handler.c \
		../ccache/src/http/request.c \
		../ccache/src/http/reply.c \
		../ccache/src/lib/util.c \
		../ccache/src/lib/sds.c \
		../ccache/src/lib/safe_queue.c \
		../ccache/src/lib/objSds.c \
		../ccache/src/lib/dicttype.c \
		../ccache/src/lib/dict.c \
		../ccache/src/lib/adlist.c \
		../ccache/src/lib/ufile.c \
		../ccache/src/lib/mhash.c \
		../ccache/src/net/client.c \
		../ccache/src/net/anet.c \
		../ccache/src/net/ae.c \
		../ccache/src/organizer/bio.c \
		../ccache/src/service/zoom.c \
		../ccache/src/net/http_server.c 
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
	@$(CHK_DIR_EXISTS) ../ccache/.tmp/ccache1.0.0 || $(MKDIR) ../ccache/.tmp/ccache1.0.0 
	$(COPY_FILE) --parents $(SOURCES) $(DIST) ../ccache/.tmp/ccache1.0.0/ && (cd `dirname ../ccache/.tmp/ccache1.0.0` && $(TAR) ccache1.0.0.tar ccache1.0.0 && $(COMPRESS) ccache1.0.0.tar) && $(MOVE) `dirname ../ccache/.tmp/ccache1.0.0`/ccache1.0.0.tar.gz . && $(DEL_FILE) -r ../ccache/.tmp/ccache1.0.0


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

main.o: ../ccache/src/main.c ../ccache/src/ccache_config.h \
		../ccache/src/organizer/bio.h \
		../ccache/src/cache/mcache.h \
		../ccache/src/signal_handler.h \
		../ccache/src/http/request_handler.h \
		../ccache/src/http/request.h \
		../ccache/src/http/reply.h \
		../ccache/src/usage.c \
		../ccache/src/net/http_server.h \
		../ccache/src/net/ae.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o main.o ../ccache/src/main.c

mcache.o: ../ccache/src/cache/mcache.c ../ccache/src/cache/mcache.h \
		../ccache/src/organizer/bio.h \
		../ccache/src/ccache_config.h \
		../ccache/src/cache/cache.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o mcache.o ../ccache/src/cache/mcache.c

cache.o: ../ccache/src/cache/cache.c ../ccache/src/cache/cache.h \
		../ccache/src/ccache_config.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o cache.o ../ccache/src/cache/cache.c

request_handler.o: ../ccache/src/http/request_handler.c ../ccache/src/http/request_handler.h \
		../ccache/src/http/request.h \
		../ccache/src/http/reply.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o request_handler.o ../ccache/src/http/request_handler.c

request.o: ../ccache/src/http/request.c ../ccache/src/http/request.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o request.o ../ccache/src/http/request.c

reply.o: ../ccache/src/http/reply.c ../ccache/src/http/reply.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o reply.o ../ccache/src/http/reply.c

util.o: ../ccache/src/lib/util.c ../ccache/src/ccache_config.h \
		../ccache/src/lib/util.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o util.o ../ccache/src/lib/util.c

sds.o: ../ccache/src/lib/sds.c ../ccache/src/lib/sds.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o sds.o ../ccache/src/lib/sds.c

safe_queue.o: ../ccache/src/lib/safe_queue.c ../ccache/src/lib/safe_queue.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o safe_queue.o ../ccache/src/lib/safe_queue.c

objSds.o: ../ccache/src/lib/objSds.c ../ccache/src/lib/objSds.h \
		../ccache/src/ccache_config.h \
		../ccache/src/lib/sds.h \
		../ccache/src/lib/adlist.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o objSds.o ../ccache/src/lib/objSds.c

dicttype.o: ../ccache/src/lib/dicttype.c ../ccache/src/lib/dicttype.h \
		../ccache/src/lib/dict.h \
		../ccache/src/lib/adlist.h \
		../ccache/src/lib/sds.h \
		../ccache/src/lib/objSds.h \
		../ccache/src/ccache_config.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o dicttype.o ../ccache/src/lib/dicttype.c

dict.o: ../ccache/src/lib/dict.c ../ccache/src/lib/dict.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o dict.o ../ccache/src/lib/dict.c

adlist.o: ../ccache/src/lib/adlist.c ../ccache/src/lib/adlist.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o adlist.o ../ccache/src/lib/adlist.c

ufile.o: ../ccache/src/lib/ufile.c ../ccache/src/lib/ufile.h \
		../ccache/src/lib/sds.h \
		../ccache/src/lib/adlist.h \
		../ccache/src/ccache_config.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o ufile.o ../ccache/src/lib/ufile.c

mhash.o: ../ccache/src/lib/mhash.c ../ccache/src/lib/mhash.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o mhash.o ../ccache/src/lib/mhash.c

client.o: ../ccache/src/net/client.c ../ccache/src/net/client.h \
		../ccache/src/ccache_config.h \
		../ccache/src/net/http_server.h \
		../ccache/src/net/ae.h \
		../ccache/src/net/anet.h \
		../ccache/src/http/request_handler.h \
		../ccache/src/http/request.h \
		../ccache/src/http/reply.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o client.o ../ccache/src/net/client.c

anet.o: ../ccache/src/net/anet.c ../ccache/src/net/anet.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o anet.o ../ccache/src/net/anet.c

ae.o: ../ccache/src/net/ae.c ../ccache/src/net/ae.h \
		../ccache/src/ccache_config.h \
		../ccache/src/net/client.h \
		../ccache/src/net/http_server.h \
		../ccache/src/cache/mcache.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o ae.o ../ccache/src/net/ae.c

bio.o: ../ccache/src/organizer/bio.c ../ccache/src/organizer/bio.h \
		../ccache/src/ccache_config.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o bio.o ../ccache/src/organizer/bio.c

zoom.o: ../ccache/src/service/zoom.c ../ccache/src/organizer/bio.h \
		../ccache/src/ccache_config.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o zoom.o ../ccache/src/service/zoom.c

http_server.o: ../ccache/src/net/http_server.c ../ccache/src/net/http_server.h \
		../ccache/src/net/ae.h \
		../ccache/src/ccache_config.h \
		../ccache/src/cache/mcache.h \
		../ccache/src/net/client.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o http_server.o ../ccache/src/net/http_server.c

####### Install

install:   FORCE

uninstall:   FORCE

FORCE:

