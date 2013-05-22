#ifndef __SERVER_H
#define __SERVER_H

#include "pthread.h"
#include "adlist.h"
#include "client.h"

/* Global server state structure */
struct redisServer {
    pthread_t mainthread;
    int arch_bits;
    int port;
    char *bindaddr;
    int ipfd;
    list *clients;
    list *slaves;
    redisClient *current_client; /* Current client, only used on crash report */
    char neterr[ANET_ERR_LEN];
    aeEventLoop *el;        /* TODO: multithreaded server using multiple eventLoop */
    int cronloops;              /* number of times the cron function run */
    int shutdown_asap;
    /* Replication related */
    int isslave;
    /* Limits */
    unsigned int maxclients;
    unsigned long long maxmemory;
};
#endif
