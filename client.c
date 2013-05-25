#include "client.h"
#include <sys/uio.h>
#include "malloc.h"
#include "anet.h"   /* Networking the easy way */
#include "pthread.h"

static int _installWriteEvent(aeEventLoop *el, httpClient *c);

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
    CCACHE_NOTUSED(el);
    CCACHE_NOTUSED(mask);
    CCACHE_NOTUSED(privdata);

    /* Accept new connection */
    char neterr[ANET_ERR_LEN];
    cfd = anetTcpAccept(neterr, fd, cip, &cport);
    if (cfd == AE_ERR) {
        rLog(CCACHE_WARNING,"Accepting client connection: %s", neterr);
        return;
    }
    rLog(CCACHE_VERBOSE,"Accepted %s:%d", cip, cport);
    /* Create a new client and add it to server.clients list */
    /* NOTICE: must be called via function */
    httpClient *c;
    /* NOTICE: not fd but cfd. */
    /* This el is from the master.
     * We may Change el to one from workers.
     */

    if ((c = createClient(nextClient(el),cfd,cip,cport)) == NULL) {
        rLog(CCACHE_WARNING,"Error allocating resoures for the client");
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
}


httpClient *createClient(aeEventLoop *el, int fd, const char* ip, int port) {
    httpClient *c = malloc(sizeof(*c));
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
    c->req = requestCreate();
    c->lastinteraction = time(NULL);
    c->ip = strdup(ip);
    c->port = port;
    c->node = listAddNodeTailGetNode(el->clients,c);
    return c;
}

void readQueryFromClient(aeEventLoop *el, int fd, void *privdata, int mask) {
    httpClient *c = (httpClient*) privdata;
    char buf[CCACHE_IOBUF_LEN];
    int nread;
    CCACHE_NOTUSED(el);
    CCACHE_NOTUSED(mask);
    nread = read(fd, buf, CCACHE_IOBUF_LEN);
    if (nread == -1) {
        if (errno == EAGAIN) { /* try again */
            nread = 0;
        } else {
            rLog(CCACHE_VERBOSE, "Reading from client: %s",strerror(errno));
            freeClient(el,c);
            return;
        }
    } else if (nread == 0) {
        rLog(CCACHE_VERBOSE, "Client closed connection");
        freeClient(el,c);
        return;
    }
    if (nread>0) {
        c->lastinteraction = time(NULL);
        listMoveNodeToTail(el->clients,c->node);
        /* NOTICE: nread or nread-1 */
        switch(requestParse(c->req,buf,buf+nread)){
        case parse_not_completed:
            break;
        case parse_completed:
            if (_installWriteEvent(el, c) != CCACHE_OK) return;
            replySetStatus(c->rep,reply_ok);
            replySetContent(c->rep,"OK");
            resetClient(c);
            break;
        case parse_error:
            if (_installWriteEvent(el, c) != CCACHE_OK) {
                return;
            }
            replySetStatus(c->rep,reply_ok);
            replySetContent(c->rep,"ERROR");
            resetClient(c);
            break;
        };
    }
}

/* Set the event loop to listen for write events on the client's socket.
 * Typically gets called every time a reply is built. */
int _installWriteEvent(aeEventLoop *el, httpClient *c) {
    if (c->fd <= 0) return CCACHE_ERR;
    if (aeCreateFileEvent(el, c->fd, AE_WRITABLE,sendReplyToClient, c) == AE_ERR)
        return CCACHE_ERR;
    return CCACHE_OK;
}

void sendReplyToClient(aeEventLoop *el, int fd, void *privdata, int mask) {
    httpClient *c = privdata;
    int nwritten = 0;

    CCACHE_NOTUSED(el);
    CCACHE_NOTUSED(mask);
    // if (totwritten > REDIS_MAX_WRITE_PER_EVENT)
    sds obuf = replyToBuffer(c->rep);
    nwritten = write(fd, obuf,sdslen(obuf));
    /* Content */
    if (nwritten == -1) {
        if (errno == EAGAIN) {
            nwritten = 0;
        } else {
            rLog(CCACHE_VERBOSE,
                 "Error writing to client: %s", strerror(errno));
            freeClient(el,c);
            return;
        }
    }
    c->lastinteraction = time(NULL);
    listMoveNodeToTail(el->clients,c->node);
    aeDeleteFileEvent(el,c->fd,AE_WRITABLE);
}

void freeClient(aeEventLoop *el, httpClient *c) {
    if(c) {
        aeDeleteFileEvent(el,c->fd,AE_READABLE);
        aeDeleteFileEvent(el,c->fd,AE_WRITABLE);
        close(c->fd);
        /* Release memory */
        if(c->ip) free(c->ip);
        replyFree(c->rep);
        requestFree(c->req);
        listDelNode(el->clients,c->node);
        free(c);
    }
}



/* resetClient prepare the client to process the next command */
void resetClient(httpClient *c) {
    replyReset(c->rep);
    requestReset(c->req);
}


#ifdef AE_MAX_IDLE_TIME
int closeTimedoutClients(aeEventLoop *el) {    
    if(el->myid != 0) {
        httpClient *c;
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
                rLog(CCACHE_VERBOSE,"Closing idle client");
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
