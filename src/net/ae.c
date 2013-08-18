/* A simple event-driven programming library. Originally I wro  te this code
 * for the Jim's event-loop (Jim is a Tcl interpreter) but later translated
 * it in form of a library for easy reuse.
 *
 * Copyright (c) 2006-2010, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#include "ae.h"
#include "client.h"
#include "cache/mcache.h"


static aeFileEvent *aeEvents = server.events;

static void unwatchClient(ccache *c) {
    cacheEntry *ce;
    while((ce=cacheGetMessage(c,CACHE_REPLY_NEW)) != NULL) {
        httpClient *client;
        list *waiting_clients = ce->waiting_clients;
        sds obuf = ce->val;
        listIter li;
        listNode *ln;
        listRewind(waiting_clients,&li);
        while ((ln = listNext(&li)) != NULL) {
            client = listNodeValue(ln);
            unblockClient(client,obuf);
            listDelNode(waiting_clients,ln);
        }
    }
}

/* We received a SIGTERM,  shuttingdown here in a safe way, as it is
 * not ok doing so inside the signal handler. */
void workerBeforeSleep(struct aeEventLoop *eventLoop){
    /*if (server.shutdown_asap) {
        if (prepareForShutdown() == CCACHE_OK) exit(0);
        redisLog(CCACHE_WARNING,"SIGTERM received but errors trying to shut down the server, check the logs for more information");
    }
    */
    unwatchClient(eventLoop->cache);    
#ifdef AE_MAX_CLIENT_IDLE_TIME
    if (eventLoop->maxidletime)
        closeTimedoutClients(eventLoop);
#endif
    if(shouldIFreeSomeData()) {
        cacheDeleteStaleEntries(eventLoop->cache,1);
    }
}

aeEventLoop *aeCreateEventLoop(void) {
    aeEventLoop *eventLoop;

    eventLoop = malloc(sizeof(*eventLoop));
    if (!eventLoop) return NULL;

    eventLoop->epfd = epoll_create(AE_MAX_EPOLL_EVENTS); /* 1024 is just an hint for the kernel */
    if (eventLoop->epfd < 0 ) {
        free(eventLoop);
        return NULL;
    }
    eventLoop->stop = 0;
    eventLoop->numfds = 0;
    eventLoop->clients = listCreate();
#ifdef AE_MAX_CLIENT_PER_WORKER
    eventLoop->maxclients = AE_MAX_CLIENT_PER_WORKER;
#endif
#ifdef AE_MAX_CLIENT_IDLE_TIME
    eventLoop->maxidletime = AE_MAX_CLIENT_IDLE_TIME;
#endif
    return eventLoop;
}

void aeDeleteEventLoop(aeEventLoop *eventLoop) {
    close(eventLoop->epfd);
    free(eventLoop);
}

void aeStop(aeEventLoop *eventLoop) {
    eventLoop->stop = 1;
}

int aeCreateFileEvent(aeEventLoop *eventLoop, int fd, int mask, void *clientData)
{    
    if (fd >= AE_FD_SET_SIZE) return AE_ERR;
    aeFileEvent *fe = aeEvents + fd;
    fe->ee->events = mask;
    /* add/modify event associated with fd to event loop */
    if (epoll_ctl(eventLoop->epfd,EPOLL_CTL_ADD,fd,fe->ee))
        return AE_ERR;
    fe->clientData = clientData;
    eventLoop->numfds++;
    printf("Add fd %d mask %d\n",fd,fe->ee->events);
    return AE_OK;
}

int aeModifyFileEvent(aeEventLoop *eventLoop, int fd, int mask, void *clientData)
{
    if (fd >= AE_FD_SET_SIZE) return AE_ERR;
    aeFileEvent *fe = aeEvents + fd;
    fe->ee->events = mask;
    /* add/modify event associated with fd to event loop */
    if (epoll_ctl(eventLoop->epfd,EPOLL_CTL_MOD,fd,fe->ee))
        return AE_ERR;
    fe->clientData = clientData;
    printf("Modify fd %d mask %d\n",fd,fe->ee->events);
    return AE_OK;
}

int aeDeleteFileEvent(aeEventLoop *eventLoop, int fd)
{
    aeFileEvent *fe = aeEvents + fd;
    fe->clientData = NULL;
    if (fe->ee->events == AE_UNACTIVATED)
        return AE_ERR; /* safe check */
    /* Note, Kernel < 2.6.9 requires a non null event pointer even for
     * EPOLL_CTL_DEL. */
    if(epoll_ctl(eventLoop->epfd,EPOLL_CTL_DEL,fd,fe->ee))
        return AE_ERR;
    fe->ee->events = AE_UNACTIVATED;
    eventLoop->numfds--;
    printf("Delete fd %d mask %d\n",fd,fe->ee->events);
    return AE_OK;
}

/*
static void aeGetTime(long *seconds, long *milliseconds)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    *seconds = tv.tv_sec;
    *milliseconds = tv.tv_usec/1000;
}
*/

void aeProcessEvents(aeEventLoop *eventLoop)
{

        int numevents = epoll_wait(eventLoop->epfd,eventLoop->newees,AE_MAX_EPOLL_EVENTS,1);
        if(numevents < 1) {
            /* No waiting client */
            usleep(10000);
            return;
        }
        struct epoll_event *newees = eventLoop->newees;
        struct epoll_event *fired_ee;
        int fd;
        aeFileEvent *fe;
        while(numevents--) {
            fired_ee = newees++;
            fd = fired_ee->data.fd;
            fe = aeEvents + fd;
            if (fired_ee->events & fe->ee->events & AE_READABLE) {
                printf("Read\n");
                readQueryFromClient(eventLoop,fd,fe->clientData);
            }
            if (fired_ee->events & fe->ee->events & AE_WRITABLE) {
                printf("Write\n");
                sendReplyToClient(eventLoop,fd,fe->clientData);
            }            
        };
}

void aeMain(aeEventLoop *eventLoop) {    
    eventLoop->stop = 0;
    while (!eventLoop->stop) {        
        workerBeforeSleep(eventLoop);
        aeProcessEvents(eventLoop);
    }
}

char *aeGetApiName(void) {
    return "epoll";
}
