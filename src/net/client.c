#include <sys/uio.h>
#include "client.h"
#include "malloc.h"
#include "anet.h"   /* Networking the easy way */
#include "pthread.h"
#include "http/request_handler.h"
#include "lib/util.h"
#include "net/http_server.h"

static int _installWriteEvent(aeEventLoop *el, httpClient *c);
static void blockClient(aeEventLoop *el, httpClient *c);



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
    c->elNode = NULL;
    c->blocked = 0;
    c->el = el;
    c->elNode = listAddNodeTailGetNode(el->clients,c);
    if (aeCreateFileEvent(el, fd, AE_READABLE, c) == AE_ERR)
    {
        printf("create client fail\n");
        freeClient(c);
        return NULL;
    }
    return c;
}


void readQueryFromClient(aeEventLoop *el, int fd, httpClient *c) {
    char buf[CCACHE_IOBUF_LEN];
    int nread;
    nread = read(fd, buf, CCACHE_IOBUF_LEN);
    if (nread == -1) {
        if (errno == EAGAIN) { /* try again */
            nread = 0;
        } else {
            ulog(CCACHE_VERBOSE, "Reading from client: %s",strerror(errno));
            freeClient(c);
            return;
        }
    } else if (nread == 0) {
        ulog(CCACHE_VERBOSE, "End of client request");
        freeClient(c);
        return;
    }
    if (nread>0) {
        printf("Read Request: %.2lf \n", (double)(clock()));
        c->lastinteraction = time(NULL);
        listMoveNodeToTail(el->clients,c->elNode);
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
                printf("Install Write: %.2lf\n", (double)(clock()));
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

void sendReplyToClient(aeEventLoop *el, int fd, httpClient *c) {
    int nwritten = 0;

    CCACHE_NOTUSED(el);    
    if(c->rep) {
        sds obuf = replyToBuffer(c->rep);
        int towrite = sdslen(obuf) - c->bufpos;
        nwritten = write(fd, obuf+c->bufpos,towrite);
        /* Content */
        if (nwritten == -1) {
            if (errno == EAGAIN) {
                nwritten = 0;
            } else {
                ulog(CCACHE_VERBOSE,
                     "Error writing to client: %s", strerror(errno));
                freeClient(c);
                return;
            }
        }
        c->lastinteraction = time(NULL);
        listMoveNodeToTail(el->clients,c->elNode);
        if(nwritten<towrite) {
            c->bufpos += nwritten;
        }
        else {
#ifdef AE_MAX_CLIENT_IDLE_TIME
            resetClient(c);
#else
            freeClient(c);
#endif
            aeModifyFileEvent(el,c->fd,AE_READABLE,c);
            printf("Send Reply: %.2lf\n", (double)(clock()));
        }
    }
}

void freeClient(httpClient *c) {
    aeDeleteFileEvent(c->el,c->fd);
    close(c->fd);
    /* Release memory */
    if(c->ip) free(c->ip);
    replyFree(c->rep);
    requestFree(c->req);
    if(c->elNode) listDelNode(c->el->clients,c->elNode);
    free(c);
}



/* resetClient prepare the client to process the next command */
void resetClient(httpClient *c) {
    c->bufpos = 0;
    replyReset(c->rep);
    requestReset(c->req);
}


#ifdef AE_MAX_CLIENT_IDLE_TIME
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
                /* the client is waiting for reply */
                if (c->blocked) {
                    /* This situation happens when request_handler time exceeds client timeout.
                     * Client timeout is typically 30 seconds and
                     * Request_handler rarely consumes more than 1 second.
                     * This rare case has a very small role in overall performance.                    
                     */
                    listNode *ln = listSearchKey(c->ceList,c);
                    if(ln) listDelNode(c->ceList,ln);
                }
                freeClient(c);
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
    if(_installWriteEvent(c->el,c) == CCACHE_OK)
        c->blocked = 0;
    c->rep->obuf = obuf;
}

/* Set the event loop to listen for write events on the client's socket.
 * Typically gets called every time a reply is built. */
int _installWriteEvent(aeEventLoop *el, httpClient *c) {
    if (c->fd <= 0) return CCACHE_ERR;
    if (aeModifyFileEvent(el,c->fd,AE_WRITABLE,c) == AE_ERR)
        return CCACHE_ERR;
    return CCACHE_OK;
}

