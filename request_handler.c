#include <stdio.h>
#include <pthread.h>
#include "request_handler.h"

static ccache *global_cache;
static pthread_mutex_t mutex_global_cache;

void requestHandleInitializeGlobalCache() {
    pthread_mutex_init(&mutex_global_cache,NULL);
    global_cache = cacheCreate();
}

int requestHandle(request *req, reply *rep, ccache *c) {
    if(c) {
        /* whether found in cache or added to cache,
         * the obuf of reply will be managed by the cache */
        rep->isCached = 1;
        printf("Cache %p\n",c);
        robj *obj = cacheFind(c,req->uri);
        if(obj) {
            if (checkType(obj,OBJ_SDS)) {
                rep->obuf = obj->ptr;
                return HANDLER_OK;
            }
            /* block client */
            return HANDLER_BLOCK;
        }
        else {
            /* not found in cache */
            printf("New entry %s \n",req->uri);
            replySetStatus(rep,reply_ok);
            replySetContent(rep,"OK");
            replyToBuffer(rep);
            sds key = sdsdup(req->uri);
            if(cacheAdd(c,key,objectCreateNULL()) == CACHE_ERR) {
                sdsfree(key);
                return HANDLER_ERR;
            }
            else {
                /* send request to global cache */
                if(cacheRequest(c,key) == CACHE_ERR) {
                    return HANDLER_ERR;
                }
                /* block client */
                return HANDLER_BLOCK;
            }
        }
    }
    return HANDLER_ERR;
}

void requestHandleError(request *req, reply *rep) {
    (void)req;
    replySetStatus(rep,reply_ok);
    replySetContent(rep,"ERROR");
}
