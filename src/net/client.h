#ifndef __CLIENT_H
#define __CLIENT_H

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include "ccache_config.h"
#include "lib/sds.h"    /* Dynamic safe strings */
#include "lib/util.h"
#include "http/reply.h"
#include "http/request.h"
#include "http_server.h"

extern httpServer server;

/*-----------------------------------------------------------------------------
 * Data types
 *----------------------------------------------------------------------------*/

typedef struct httpClient {
    int fd;    
    char *ip;
    int port;
    reply *rep;
    int bufpos;
    request *req;
    time_t lastinteraction; /* time of the last interaction, used for timeout */
    listNode *elNode; /* point to the position this clients in its eventLoop's list of clients*/
    int blocked;
    aeEventLoop *el;
    list *ceList; /* point to the position of this clients in cache waiting list */
} httpClient;

typedef struct {
    int fd;
    char *ip;
    int port;
    int wasAccepted;
} clientEndpoint;


/*-----------------------------------------------------------------------------
 * Functions prototypes
 *----------------------------------------------------------------------------*/

httpClient *createClient(aeEventLoop *el, int fd, const char *ip, int port);
#ifdef AE_MAX_CLIENT_IDLE_TIME
int closeTimedoutClients(aeEventLoop *el);
#endif

void freeClient(httpClient *c);
void resetClient(httpClient *c);
void sendReplyToClient(aeEventLoop *el, int fd, httpClient *c);
void readQueryFromClient(aeEventLoop *el, int fd, httpClient *c);

void unblockClient(httpClient *c, sds obuf);

/*
Use other mem allocator? NO

#if defined(__GNUC__)
void *calloc(size_t count, size_t size) __attribute__ ((deprecated));
void free(void *ptr) __attribute__ ((deprecated));
void *malloc(size_t size) __attribute__ ((deprecated));
void *realloc(void *ptr, size_t size) __attribute__ ((deprecated));
#endif
*/

#endif
