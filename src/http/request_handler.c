/* request_handler.c
 *
 * Copyright (c) 2013, Nguyen Truong Minh <nguyentrminh at gmail dot com>
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
#include <pthread.h>
#include "request_handler.h"
#include "lib/util.h"

static ccache *global_cache;
static pthread_mutex_t mutex_global_cache;

void requestHandleInitializeGlobalCache() {
    pthread_mutex_init(&mutex_global_cache,NULL);
    global_cache = cacheCreate();
}

int requestHandle(request *req, reply *rep, ccache *c, void *client) {
    if(c) {
        /* whether found in cache or newly added to cache,
         * the obuf of reply will be managed by the cache */
        replyToBeCached(rep);
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
