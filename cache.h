#ifndef CCACHE_H
#define CCACHE_H

#include "dict.h"
#include "adlist.h"
#include "object.h"
#include "safe_queue.h"
#include "client.h"

#define CACHE_OK DICT_OK
#define CACHE_ERR DICT_ERR

#define CACHE_NEW 1
#define CACHE_OLD 2

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

void cacheMasterInit();
ccache *cacheAddSlave(void *el);
void cacheAddWatchClient(cacheEntry *ce, httpClient *client);

#endif // CCACHE_H
