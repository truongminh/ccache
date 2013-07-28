/* cache.h - A LRU cache implementation
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
 *   * Neither the name of CCACHE nor the names of its contributors may be used
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

#ifndef CCACHE_H
#define CCACHE_H

#include "dict.h"
#include "adlist.h"
#include "safe_queue.h"
#include "client.h"

#define CACHE_OK DICT_OK
#define CACHE_ERR DICT_ERR

#define CACHE_NEW 1
#define CACHE_OLD 2

list *slave_caches;

typedef struct cacheEntry {
    dictEntry *de;
    listNode *ln;
    void *val;
    safeQueue *waiting_clients;
} cacheEntry;

typedef struct {
    dict *data;
    safeQueue *forOld;
    safeQueue *forNew;
    list *accesslist;    
    void *el;
} ccache;

#define cacheGetList(c) (c->accesslist)
ccache *cacheCreate();
int cacheDeleteStaleEntries(ccache *c, unsigned int n);
void cacheDelete(ccache* c, sds key);
cacheEntry *cacheAdd(ccache *c, sds key, void *value);
cacheEntry *cacheFind(ccache *c, sds key);
void *cacheFetch(ccache *c, sds key);
#define cacheNumberOfEntry(c) (c->used)

int cacheRequest(ccache *c, sds key);
int cacheSendMessage(ccache *c, void *ce, int forWhom);
void *cacheGetMessage(ccache *c, int forWhom);

ccache *cacheAddSlave(void *el);
void cacheAddWatchClient(cacheEntry *ce, httpClient *client);


#if(CCACHE_LOG_LEVEL == CCACHE_DEBUG)
    #define REPORT_MASTER_ADD_KEY(key) printf("Master \t Add new entry [%s]\n",key)
#else
    #define REPORT_MASTER_ADD_KEY(key) ; /* just ignore */
#endif


#endif // CCACHE_H
