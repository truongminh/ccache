/* Linux epoll(2) based ae.c module
 * Copyright (C) 2009-2010 Salvatore Sanfilippo - antirez@gmail.com
 * Released under the BSD license. See the COPYING file for more info. */

/* This epoll API is based on 2 components:
 * (1) An fd tracking of fired events
 * (2) A queue of fired events
 */

#include <sys/epoll.h>

typedef struct aeApiState {
    int epfd; // epoll file descriptor. Note: fd is simply an int
    // our server has only one epoll instance.
    struct epoll_event events[AE_SETSIZE];
} aeApiState;

static int aeApiCreate(aeEventLoop *eventLoop) {
    aeApiState *state = malloc(sizeof(aeApiState));

    if (!state) return -1;
    state->epfd = epoll_create(1024); /* 1024 is just an hint for the kernel */
    if (state->epfd == -1) return -1;
    eventLoop->apidata = state;
    return 0;
}

static void aeApiFree(aeEventLoop *eventLoop) {
    aeApiState *state = eventLoop->apidata;

    close(state->epfd);
    free(state);
}

/**********************************************************************/
static int aeApiAddEvent(aeEventLoop *eventLoop, int fd, int mask) {
    aeApiState *state = eventLoop->apidata;
    struct epoll_event ee;
    /* If the fd was already monitored for some event, we need a MOD
     * operation. Otherwise we need an ADD operation. */
    int op = eventLoop->events[fd].mask == AE_NONE ?
            EPOLL_CTL_ADD : EPOLL_CTL_MOD;

    ee.events = 0;
    mask |= eventLoop->events[fd].mask; /* Merge old events */
    if (mask & AE_READABLE) ee.events |= EPOLLIN;
    if (mask & AE_WRITABLE) ee.events |= EPOLLOUT;
    ee.data.u64 = 0; /* avoid valgrind warning */
    /* to retrieve fd when the event is fired */
    ee.data.fd = fd;
    /* add/modify event associated with fd to event loop */
    if (epoll_ctl(state->epfd,op,fd,&ee) == -1) return -1;
    return 0;
}

static void aeApiDelEvent(aeEventLoop *eventLoop, int fd, int delmask) {
    aeApiState *state = eventLoop->apidata;
    struct epoll_event ee;
    int mask = eventLoop->events[fd].mask & (~delmask);

    ee.events = 0;
    if (mask & AE_READABLE) ee.events |= EPOLLIN;
    if (mask & AE_WRITABLE) ee.events |= EPOLLOUT;
    ee.data.u64 = 0; /* avoid valgrind warning */
    ee.data.fd = fd;
    if (mask != AE_NONE) {
        epoll_ctl(state->epfd,EPOLL_CTL_MOD,fd,&ee);
    } else {
        /* Note, Kernel < 2.6.9 requires a non null event pointer even for
         * EPOLL_CTL_DEL. */
        epoll_ctl(state->epfd,EPOLL_CTL_DEL,fd,&ee);
    }
}

/* There are three predefined file descriptors: STDIN, STDOUT, STDERR */
static int aeApiPoll(aeEventLoop *eventLoop, struct timeval *tvp) {
    aeApiState *state = eventLoop->apidata;
    int retval, numevents = 0;
    /* If tvp is 0, the call return immediately
       If tvp = NULL then tvp = -1, the call will not return until an event is available
     */
    retval = epoll_wait(state->epfd,state->events,AE_SETSIZE,
                        // time out is in milliseconds
            tvp ? (tvp->tv_sec*1000 + tvp->tv_usec/1000) : -1);    
    if (retval > 0) {
        /* All Occured Events is in state->events array
            Only fd and mask is extracted
         */
        int j;        

        numevents = retval;
        for (j = 0; j < numevents; j++) {
            int mask = 0;
            struct epoll_event *e = state->events+j;

            if (e->events & EPOLLIN) mask |= AE_READABLE;
            if (e->events & EPOLLOUT) mask |= AE_WRITABLE;
            eventLoop->fired[j].fd = e->data.fd;
            eventLoop->fired[j].mask = mask;
        }
    }            
    return numevents;
}

static char *aeApiName(void) {
    return "epoll";
}
