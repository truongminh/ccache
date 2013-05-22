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

/* Error codes */
#define REDIS_OK                0
#define REDIS_ERR               -1

/* Static server configuration */
#define REDIS_SERVERPORT        6379    /* TCP port */
#define REDIS_MAXIDLETIME       0       /* default client timeout: infinite */
#define REDIS_IOBUF_LEN         (1024*4)

#define REDIS_REPL_TIMEOUT 60
#define REDIS_REPL_PING_SLAVE_PERIOD 10

#define REDIS_ENCODING_RAW 0     /* Raw representation */
#define REDIS_ENCODING_INT 1     /* Encoded as integer */

/* Hash table parameters */
#define REDIS_HT_MINFILL        10      /* Minimal hash table fill 10% */


/* Log levels */
#define REDIS_DEBUG 0
#define REDIS_VERBOSE 1
#define REDIS_NOTICE 2
#define REDIS_WARNING 3


/* Object types
   The types are used when we free the objects
*/
#define REDIS_STRING 0
#define REDIS_LIST 1
#define REDIS_SET 2
#define REDIS_HASH 4

typedef struct redisObject {
    unsigned type:4;
    unsigned storage:2;     /* REDIS_VM_MEMORY or REDIS_VM_SWAPPING */
    unsigned encoding:4;
    unsigned lru:22;        /* lru time (relative to server.lruclock) */
    int refcount;
    void *ptr;
    /* VM fields are only allocated if VM is active, otherwise the
     * object allocation function will just allocate
     * sizeof(redisObjct) minus sizeof(redisObjectVM), so using
     * Redis without VM active will not have any overhead. */
} robj;

/* Anti-warning macro... */
#define REDIS_NOTUSED(V) ((void) V)

/*-----------------------------------------------------------------------------
 * Data types
 *----------------------------------------------------------------------------*/

/* With multiplexing we need to take per-clinet state.
 * Clients are taken in a liked list. */
typedef struct redisClient {
    int fd;    
    char *ip;
    int port;
    int dictid; /* What is it? */
    sds querybuf;
    reply *rep;
    int reqtype;
    unsigned long reply_bytes; /* Tot bytes of objects in reply list */
    int sentlen;
    time_t lastinteraction; /* time of the last interaction, used for timeout */
    listNode *node;
} redisClient;

/*-----------------------------------------------------------------------------
 * Extern declarations
 *----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 * Functions prototypes
 *----------------------------------------------------------------------------*/

/* networking.c -- Networking and Client related operations */
redisClient *createClient(aeEventLoop *el, int fd);
#ifdef AE_MAX_IDLE_TIME
int closeTimedoutClients(aeEventLoop *el);
#endif

void freeClient(aeEventLoop *el, redisClient *c);
void resetClient(redisClient *c);
void sendReplyToClient(aeEventLoop *el, int fd, void *privdata, int mask);
void processInputBuffer(redisClient *c);
void acceptTcpHandler(aeEventLoop *el, int fd, void *privdata, int mask);
void readQueryFromClient(aeEventLoop *el, int fd, void *privdata, int mask);

/*
#if defined(__GNUC__)
void *calloc(size_t count, size_t size) __attribute__ ((deprecated));
void free(void *ptr) __attribute__ ((deprecated));
void *malloc(size_t size) __attribute__ ((deprecated));
void *realloc(void *ptr, size_t size) __attribute__ ((deprecated));
#endif
*/

#endif
