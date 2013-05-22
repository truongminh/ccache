#include "client.h"
#include <sys/uio.h>
#include "malloc.h"
#include "anet.h"   /* Networking the easy way */
#include "pthread.h"

static int _installWriteEvent(aeEventLoop *el, redisClient *c);

/* -----------------------------------------------------------------------------
 * Low level functions to accept connection and create new clients
 * -------------------------------------------------------------------------- */
static aeEventLoop *nextClient(aeEventLoop* el){
    int current_id = el->nextSlaveID;
    el->nextSlaveID++;
    if (el->nextSlaveID == el->numslave) el->nextSlaveID = 0;
    return el->slaves[current_id];
}

void acceptTcpHandler(aeEventLoop *el, int fd, void *privdata, int mask) {
    int cport, cfd;
    char cip[128];
    REDIS_NOTUSED(el);
    REDIS_NOTUSED(mask);
    REDIS_NOTUSED(privdata);

    /* Accept new connection */
    char neterr[ANET_ERR_LEN];
    cfd = anetTcpAccept(neterr, fd, cip, &cport);
    if (cfd == AE_ERR) {
        rLog(REDIS_WARNING,"Accepting client connection: %s", neterr);
        return;
    }
    rLog(REDIS_VERBOSE,"Accepted %s:%d", cip, cport);
    /* Create a new client and add it to server.clients list */    
    /* NOTICE: must be called via function */
    redisClient *c;
    /* NOTICE: not fd but cfd. */
    /* This el is from the master.
     * We may Change el to one from workers.
     */

    if ((c = createClient(nextClient(el),cfd)) == NULL) {
        rLog(REDIS_WARNING,"Error allocating resoures for the client");
        close(cfd); /* May be already closed, just ingore errors */
        return;
    }
#ifdef AE_MAX_CLIENT
    /* Check for max client */
    if (el->maxclients && listLength(el->clients) > el->maxclients) {
        char *err = "-ERR max number of clients reached\r\n";
        /* That's a best effort error message, don't check write errors */
        if (write(cfd,err,strlen(err)) == -1) {
            /* Nothing to do, Just to avoid the warning... */
        }
        freeClient(el,c);
        return;
    }
#endif
    c->ip = strdup(cip);
    c->port = cport;
}


redisClient *createClient(aeEventLoop *el, int fd) {
    redisClient *c = malloc(sizeof(*c));
    /* Set the socket to nonblock as the default state set by the kernel is blocking or waiting */
    anetNonBlock(NULL,fd);
    anetTcpNoDelay(NULL,fd);

    if (aeCreateFileEvent(el,fd,AE_READABLE,
        readQueryFromClient, c) == AE_ERR)
    {
        close(fd);
        free(c);
        return NULL;
    }

    c->fd = fd;
    c->rep = replyCreate();
    c->querybuf = sdsempty();
    c->lastinteraction = time(NULL);
    c->reply_bytes = 0;
    c->ip = NULL;
    c->node = listAddNodeTailGetNode(el->clients,c);
    return c;
}

void readQueryFromClient(aeEventLoop *el, int fd, void *privdata, int mask) {
    redisClient *c = (redisClient*) privdata;
        char buf[REDIS_IOBUF_LEN];
        int nread;
        REDIS_NOTUSED(el);
        REDIS_NOTUSED(mask);
        nread = read(fd, buf, REDIS_IOBUF_LEN);
        if (nread == -1) {
            if (errno == EAGAIN) { /* try again */
                nread = 0;
            } else {
                rLog(REDIS_VERBOSE, "Reading from client: %s",strerror(errno));
                freeClient(el,c);
                return;
            }
        } else if (nread == 0) {
            rLog(REDIS_VERBOSE, "Client closed connection");
            freeClient(el,c);
            return;
        }
        if (nread>100) {
            /* Read data and store it to querybuf */
            c->querybuf = sdscatlen(c->querybuf,buf,100);
            c->lastinteraction = time(NULL);
        }

        if (_installWriteEvent(el, c) != REDIS_OK) {
            return;
        }
        /* parse buffer, then process the command */
        //processInputBuffer(c);
        replySetStatus(c->rep,reply_ok);
        replySetContent(c->rep,"Hello World!");

}

/* Set the event loop to listen for write events on the client's socket.
 * Typically gets called every time a reply is built. */
int _installWriteEvent(aeEventLoop *el, redisClient *c) {
    if (c->fd <= 0) return REDIS_ERR;
    //printf("Install event\n");
    if (aeCreateFileEvent(el, c->fd, AE_WRITABLE,sendReplyToClient, c) == AE_ERR)
        return REDIS_ERR;
    return REDIS_OK;
}

void sendReplyToClient(aeEventLoop *el, int fd, void *privdata, int mask) {
    redisClient *c = privdata;
    int nwritten = 0;

    REDIS_NOTUSED(el);
    REDIS_NOTUSED(mask);
    printf("Sending reply to client \n");
    sds obuf = replyToBuffer(c->rep);
    nwritten = write(fd, obuf,sdslen(obuf));
    /* Content */
    if (nwritten == -1) {
        if (errno == EAGAIN) {
            nwritten = 0;
        } else {
            rLog(REDIS_VERBOSE,
                "Error writing to client: %s", strerror(errno));
            freeClient(el,c);
            return;
        }
    }
    c->lastinteraction = time(NULL);
    aeDeleteFileEvent(el,c->fd,AE_WRITABLE);
}

void freeClient(aeEventLoop *el, redisClient *c) {
    if(c) {
        aeDeleteFileEvent(el,c->fd,AE_READABLE);
        aeDeleteFileEvent(el,c->fd,AE_WRITABLE);
        close(c->fd);
        /* Release memory */
        if(c->ip) free(c->ip);
        replyFree(c->rep);
        free(c);
    }
}



/* resetClient prepare the client to process the next command */
void resetClient(redisClient *c) {
    REDIS_NOTUSED(c);
    //freeClientArgv(c);
}

void processInputBuffer(redisClient *c) {
    /* Split arguments */        
    c->querybuf = sdsempty();

}


#ifdef AE_MAX_IDLE_TIME
int closeTimedoutClients(aeEventLoop *el) {    
    if(el->myid != 0) {
        redisClient *c;
        printf("List %d\n",listLength(el->clients));
        int deletedNodes = 0;
        time_t now = time(NULL);        
        listIter li;
        listNode *ln;

        listRewind(el->clients,&li);
        while ((ln = listNext(&li)) != NULL) {
            c = listNodeValue(ln);
            if (el->maxidletime &&
                (now - c->lastinteraction > el->maxidletime))
            {
                rLog(REDIS_VERBOSE,"Closing idle client");
                listDelNode(el->clients,c->node);
                freeClient(el,c);
                deletedNodes++;
            }
            else break;
        }
        return deletedNodes;
    }
    return 0;
}

#endif
