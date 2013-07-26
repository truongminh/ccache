#include "client.h"
#include <sys/uio.h>
#include "malloc.h"
#include "anet.h"   /* Networking the easy way */
#include "pthread.h"
#include "request_handler.h"
#include "util.h"

static int _installWriteEvent(aeEventLoop *el, httpClient *c);
static void blockClient(aeEventLoop *el, httpClient *c);
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
        ulog(CCACHE_WARNING,"Accepting client connection: %s", neterr);
        return;
    }
    ulog(CCACHE_DEBUG,"Accepted %s:%d", cip, cport);
    /* Create a new client and add it to server.clients list */
    /* NOTICE: must be called via function */
    httpClient *c;
    /* NOTICE: not fd but cfd. */
    /* This el is from the master.
     * We may Change el to one from workers.
     */
    aeEventLoop *nextEL = nextClient(el);    
    if ((c = createClient(nextEL,cfd,cip,cport)) == NULL) {
        ulog(CCACHE_WARNING,"Error allocating resoures for the client");
        freeClient(nextEL,c); /* May be already closed, just ingore errors */
        return;
    }
#ifdef AE_MAX_CLIENT
    /* Check for max client */
    if (nextEL->maxclients && listLength(nextEL->clients) > nextEL->maxclients) {
        char *err = "-ERR max number of clients reached\r\n";
        /* That's a best effort error message, don't check write errors */
        if (write(cfd,err,strlen(err)) == -1) {
            /* Nothing to do, Just to avoid the warning... */
        }
        freeClient(nextEL,c);
        return;
    }
#endif
}


httpClient *createClient(aeEventLoop *el, int fd, const char* ip, int port) {
    httpClient *c = malloc(sizeof(*c));
    /* Set the socket to nonblock as the default state set by the kernel is blocking or waiting */
    anetNonBlock(NULL,fd);
    anetTcpNoDelay(NULL,fd);
    c->fd = fd;
    c->rep = replyCreate();
    c->bufpos = 0;
    c->req = requestCreate();
    c->lastinteraction = time(NULL);
    c->ip = strdup(ip);
    c->port = port;
    c->node = NULL;
    c->blocked = 0;
    c->el = el;
    if (aeCreateFileEvent(el,fd,AE_READABLE,readQueryFromClient, c) == AE_ERR)
    {
        close(fd);
        freeClient(el,c);
        return NULL;
    }
    return c;
}

void readQueryFromClient(aeEventLoop *el, int fd, void *privdata, int mask) {
    httpClient *c = (httpClient*) privdata;
    if(c->node == NULL) c->node = listAddNodeTailGetNode(el->clients,c);
    char buf[CCACHE_IOBUF_LEN];
    int nread;
    CCACHE_NOTUSED(mask);
    nread = read(fd, buf, CCACHE_IOBUF_LEN);
    if (nread == -1) {
        if (errno == EAGAIN) { /* try again */
            nread = 0;
        } else {
            ulog(CCACHE_VERBOSE, "Reading from client: %s",strerror(errno));
            freeClient(el,c);
            return;
        }
    } else if (nread == 0) {
        ulog(CCACHE_VERBOSE, "Client closed connection");
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
        {
            int handle_result = requestHandle(c->req,c->rep,el->cache,c);
            if(handle_result == HANDLER_BLOCK){
                blockClient(el,c);
            }
            else {
                if (_installWriteEvent(el, c) != CCACHE_OK) return;
                /* For HANDLE_OK there is nothing to do */
                if(handle_result == HANDLER_ERR) requestHandleError(c->req,c->rep);
            }
                break;
        }
        case parse_error:
            if (_installWriteEvent(el, c) != CCACHE_OK) {
                return;
            }
            requestHandleError(c->req,c->rep);
            break;
        default:
            break;
        };
    }
}

void sendReplyToClient(aeEventLoop *el, int fd, void *privdata, int mask) {
    httpClient *c = privdata;
    int nwritten = 0;

    CCACHE_NOTUSED(el);
    CCACHE_NOTUSED(mask);
    // if (totwritten > REDIS_MAX_WRITE_PER_EVENT)
    sds obuf = replyToBuffer(c->rep);
    int towrite = sdslen(obuf) - c->bufpos;
    if(obuf) {
        nwritten = write(fd, obuf+c->bufpos,towrite);
        /* Content */
        if (nwritten == -1) {
            if (errno == EAGAIN) {
                nwritten = 0;
            } else {
                ulog(CCACHE_VERBOSE,
                     "Error writing to client: %s", strerror(errno));
                freeClient(el,c);
                return;
            }
        }
        c->lastinteraction = time(NULL);
        listMoveNodeToTail(el->clients,c->node);
        if(nwritten<towrite) {
            c->bufpos += nwritten;
        }
        else {
            resetClient(c);
            aeDeleteFileEvent(el,c->fd,AE_WRITABLE);
        }
    }
}

void freeClient(aeEventLoop *el, httpClient *c) {
    aeDeleteFileEvent(el,c->fd,AE_READABLE);
    aeDeleteFileEvent(el,c->fd,AE_WRITABLE);
    close(c->fd);
    /* Release memory */
    if(c->ip) free(c->ip);
    replyFree(c->rep);
    requestFree(c->req);
    if(c->node) listDelNode(el->clients,c->node);
    free(c);
}



/* resetClient prepare the client to process the next command */
void resetClient(httpClient *c) {
    c->bufpos = 0;
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
                ulog(CCACHE_VERBOSE,"Closing idle client");
                /* if (c->isblocked) DONT FREE CLIENT */
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

void blockClient(aeEventLoop *el, httpClient *c)
{
    (void)el;
    /*Problem with Client Connection: aeDeleteFileEvent(el,c->fd,AE_READABLE);*/
    c->blocked = 1;    
}

void unblockClient(httpClient *c, sds obuf)
{
    /* CRITICAL: Possible Data Race Conidtion */
    if(_installWriteEvent(c->el,c) == CCACHE_OK)
        c->blocked = 0;
    c->rep->obuf = obuf;
}

/* Set the event loop to listen for write events on the client's socket.
 * Typically gets called every time a reply is built. */
int _installWriteEvent(aeEventLoop *el, httpClient *c) {
    if (c->fd <= 0) return CCACHE_ERR;
    if (aeCreateFileEvent(el, c->fd, AE_WRITABLE,sendReplyToClient, c) == AE_ERR)
        return CCACHE_ERR;
    return CCACHE_OK;
}

