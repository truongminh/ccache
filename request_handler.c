#include <stdio.h>
#include <pthread.h>
#include "request_handler.h"

static ccache *global_cache;
static pthread_mutex_t mutex_global_cache;

void requestHandleInitializeGlobalCache() {
    pthread_mutex_init(&mutex_global_cache,NULL);
    global_cache = cacheCreate();
}

int requestHandle(request *req, reply *rep, ccache *c, void *client) {
    if(c) {
        /* whether found in cache or added to cache,
         * the obuf of reply will be managed by the cache */
        rep->isCached = 1;
        cacheEntry *ce = cacheFind(c,req->uri);
        if(ce) {
            if (ce->val) {
                rep->obuf = ce->val;
                return HANDLER_OK;
            }
            /* NULL object */
            cacheAddWatchClient(ce,client);
            /* block client */
            return HANDLER_BLOCK;
        }
    }
    return HANDLER_ERR;
}

void requestHandleError(request *req, reply *rep) {
    (void)req;
    replySetStatus(rep,reply_ok);
    replySetContent(rep,"ERROR");
}
