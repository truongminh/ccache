#ifndef __CLIENT_H
#define __CLIENT_H


#if defined(__sun)
#include "solarisfixes.h"
#endif

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>

#include "ae.h"     /* Event driven programming library */
#include "sds.h"    /* Dynamic safe strings */
#include "util.h"
#include "reply.h"
#include "request.h"
#include "object.h"

/* Error codes */
#define CCACHE_OK                0
#define CCACHE_ERR               -1

/* Static server configuration */
#define CCACHE_SERVERPORT        6379    /* TCP port */
#define CCACHE_MAXIDLETIME       0       /* default client timeout: infinite */
#define CCACHE_IOBUF_LEN         (1024*4)

#define CCACHE_ENCODING_RAW 0     /* Raw representation */
#define CCACHE_ENCODING_INT 1     /* Encoded as integer */

/* Hash table parameters */
#define CCACHE_HT_MINFILL        10      /* Minimal hash table fill 10% */

/* Log levels */
#define CCACHE_DEBUG 0
#define CCACHE_VERBOSE 1
#define CCACHE_NOTICE 2
#define CCACHE_WARNING 3
#define CCACHE_LOG_LEVEL 0

/* Anti-warning macro... */
#define CCACHE_NOTUSED(V) ((void) V)

/*-----------------------------------------------------------------------------
 * Data types
 *----------------------------------------------------------------------------*/

typedef struct httpClient {
    int fd;    
    char *ip;
    int port;
    reply *rep;
    request *req;
    time_t lastinteraction; /* time of the last interaction, used for timeout */
    listNode *node; /* point to the position this clients in its eventLoop's list of clients*/
    int blocked;
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
#ifdef AE_MAX_IDLE_TIME
int closeTimedoutClients(aeEventLoop *el);
#endif

void freeClient(aeEventLoop *el, httpClient *c);
void resetClient(httpClient *c);
void sendReplyToClient(aeEventLoop *el, int fd, void *privdata, int mask);
void acceptTcpHandler(aeEventLoop *el, int fd, void *privdata, int mask);
void readQueryFromClient(aeEventLoop *el, int fd, void *privdata, int mask);

void unblockClient(aeEventLoop *el, httpClient *c, sds obuf);

/*
#if defined(__GNUC__)
void *calloc(size_t count, size_t size) __attribute__ ((deprecated));
void free(void *ptr) __attribute__ ((deprecated));
void *malloc(size_t size) __attribute__ ((deprecated));
void *realloc(void *ptr, size_t size) __attribute__ ((deprecated));
#endif
*/

#endif
