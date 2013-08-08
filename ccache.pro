TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

HEADERS += \
    src/signal_handler.h \
    src/ccache_config.h \
    src/cache/mcache.h \
    src/cache/cache.h \
    src/http/request_handler.h \
    src/http/request.h \
    src/http/reply.h \
    src/lib/util.h \
    src/lib/sds.h \
    src/lib/safe_queue.h \
    src/lib/objSds.h \
    src/lib/dicttype.h \
    src/lib/dict.h \
    src/lib/adlist.h \
    src/lib/ufile.h \
    src/lib/mhash.h \
    src/net/client.h \
    src/net/anet.h \
    src/net/ae.h \    
    src/organizer/bio.h \
    src/service/img.h

SOURCES += \
    src/main.c \    
    src/cache/mcache.c \
    src/cache/cache.c \
    src/http/request_handler.c \
    src/http/request.c \
    src/http/reply.c \
    src/lib/util.c \
    src/lib/sds.c \
    src/lib/safe_queue.c \
    src/lib/objSds.c \
    src/lib/dicttype.c \
    src/lib/dict.c \
    src/lib/adlist.c \
    src/lib/ufile.c \
    src/lib/mhash.c \
    src/net/client.c \
    src/net/anet.c \
    src/net/ae_epoll.c \
    src/net/ae.c \
    src/organizer/bio.c \
    src/service/img.c


INCLUDEPATH +=  src/
INCLUDEPATH += /usr/local/include/opencv

LIBS += -L/usr/local/lib/ -lopencv_core -lopencv_highgui -lopencv_imgproc
LIBS += -L/usr/lib/ -lpthread

# DEBUG
LIBS += -L/usr/local/lib/ -lopencv_legacy




















