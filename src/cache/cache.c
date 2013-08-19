/* cache.c - A LRU slave cache implementation.
 * For info about master cache, see mcache.c.
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


#include <unistd.h>
#include "cache.h"
#include "malloc.h"
#include "string.h" /* for memcpy */
#include "lib/dicttype.h"
#include "lib/dict.h"
#include "lib/adlist.h"
#include "ccache_config.h"

/*
* long page_size = sysconf (_SC_PAGESIZE);
*/

list *slave_caches = NULL;

ccache *cacheAddSlave(void *el) {
    ccache *c = cacheCreate();
    c->el = el;
    listAddNodeTail(slave_caches,c);
    return c;
}

void dictCacheEntryDestructor(void *privdata, void *val)
{
    DICT_NOTUSED(privdata);    
    free(val);
}


/* Command table. sds string -> command pointer. */
dictType ccacheType = {
    dictSdsHash,           /* hash function */
    NULL,                      /* key dup */
    NULL,                      /* val dup */
    dictSdsKeyCompare,     /* key compare */
    dictSdsDestructor,         /* key destructor */
    dictCacheEntryDestructor /* val destructor */
};

ccache *cacheCreate() {
    ccache *c = malloc(sizeof(*c));
    c->accesslist = listCreate();
    c->data = dictCreate(&ccacheType,NULL);
    dictExpand(c->data,PRESERVED_CACHE_ENTRIES);
    c->outboxOld = safeQueueCreate();
    c->outboxNew = safeQueueCreate();
    c->inboxNew = safeQueueCreate();
    return c;
}


cacheEntry *cacheAdd(ccache *c, sds key, void *value) {
    cacheEntry *ce;
    if ((ce = malloc(sizeof(*ce))) == NULL)
        return NULL;
    ce->ln = listAddNodeTailGetNode(c->accesslist,key);
    if(!ce->ln) {
        free(ce);
        return NULL;
    }
    ce->de = dictAddGetDictEntry(c->data,key,ce);
    if(!ce->de) {
        listDelNode(c->accesslist,ce->ln);
        free(ce);
        return NULL;
    }
    ce->waiting_clients = listCreate();
    ce->de->val = ce;
    ce->val = value;
    ce->mycache = c;
    return ce;
}

void *cacheFetch(ccache *c, sds key) {
    cacheEntry *ce = (cacheEntry *)dictFetchValue(c->data,key);
    if(ce) {
        listMoveNodeToTail(c->accesslist,ce->ln);
        return ce->val;
    }
    return NULL;
}

cacheEntry *cacheFind(ccache *c, sds key) {
    cacheEntry *ce = dictFetchValue(c->data,key);
    if(ce == NULL) {
        ce = cacheAdd(c,sdsdup(key),NULL);
        cacheSendMessage(c,ce,CACHE_REQUEST_NEW);
    }
    return ce;
}

void cacheDelete(ccache* c, sds key) {
    cacheEntry *ce = (cacheEntry *)dictFetchValue(c->data,key);
    /* Master reply by setting val to an object.
     * We do not delete cache entry until the master reply
     */
    if(ce&&ce->val)
    {
        cacheSendMessage(c,sdsdup(key),CACHE_REQUEST_OLD);
        listDelNode(c->accesslist,ce->ln);
        dictDelete(c->data,key);
    }
}

int cacheDeleteStaleEntries(ccache *c, unsigned int n) {    
    unsigned int remain = n;
    listIter li;
    listNode *ln;

    listRewind(c->accesslist,&li);
    while (remain-- && (ln = listNext(&li)) != NULL) {
        sds key = listNodeValue(ln);
        cacheDelete(c,key);
    }
    /* Return the number of successfully deleted entries */
    return n - remain;
}

int cacheSendMessage(ccache *c, void *msg, int forWhom){
    switch(forWhom) {
        case CACHE_REQUEST_NEW:
            return (safeQueuePush(c->outboxNew,msg) == SAFE_QUEUE_OK)? CACHE_OK:CACHE_ERR;
        case CACHE_REQUEST_OLD:
            return (safeQueuePush(c->outboxOld,msg) == SAFE_QUEUE_OK)? CACHE_OK:CACHE_ERR;
        case CACHE_REPLY_NEW:
            return (safeQueuePush(c->inboxNew,msg) == SAFE_QUEUE_OK)? CACHE_OK:CACHE_ERR;
        default:
            return CACHE_ERR;
    }
}

void *cacheGetMessage(ccache *c, int forWhom){
    switch(forWhom) {
        case CACHE_REQUEST_NEW:
            return safeQueuePop(c->outboxNew);
        case CACHE_REQUEST_OLD:
            return safeQueuePop(c->outboxOld);
        case CACHE_REPLY_NEW:
            return safeQueuePop(c->inboxNew);
        default:
            return NULL;
    }
}

