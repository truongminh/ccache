
SOURCES += \
    main.c \
    dict.c \
    ae_epoll.c \
    ae.c \
    adlist.c \
    zmalloc.c \
    anet.c \
    util.c \
    sds.c \
    object.c \
    reply.c \
    dicttype.c \
    client.c \
    request.c \
    cache.c \
    request_handler.c \
    objSds.c \
    safe_queue.c \
    ufile.c \
    img.c \
    bio.c

HEADERS += \
    dict.h \
    ae.h \
    adlist.h \
    zmalloc.h \
    anet.h \
    util.h \
    sds.h \
    reply.h \
    dicttype.h \
    object.h \
    client.h \
    signal_handler.h \
    server.h \
    request.h \
    cache.h \
    request_handler.h \
    objSds.h \
    safe_queue.h \
    ufile.h \
    img.h \
    bio.h \
    ccache_config.h


INCLUDEPATH += /usr/local/include/opencv

LIBS += -L/usr/local/lib/ -lopencv_core -lopencv_highgui
# DEBUG
LIBS += -L/usr/local/lib/ -lopencv_legacy





















