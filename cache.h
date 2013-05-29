#ifndef CCACHE_H
#define CCACHE_H

#include "dict.h"
#include "adlist.h"
#include "object.h"
#include "safe_list.h"


#define CACHE_OK DICT_OK
#define CACHE_ERR DICT_ERR

typedef struct cacheEntry {
    listNode *ln;
    robj *value;
} cacheEntry;

typedef struct {
    dict *d;
    safeList *in;
    safeList *out;
    list *accesslist;
} ccache;

typedef struct {
    sds key;
    robj *obj;
} cacheMsg;

#define cacheGetList(c) (c->accesslist)
ccache *cacheCreate();
int cacheDeleteStaleEntries(ccache *c, unsigned int n);
void cacheDelete(ccache* c, sds key);
int cacheAdd(ccache *c, sds key, robj *value);
robj *cacheFind(ccache *c, sds key);
#define cacheNumberOfEntry(c) (c->used)

int cacheRequest(ccache *c, sds key);
int cacheSendMessage(ccache *c, robj *obj);
cacheMsg *cacheGetMessage(ccache *c);

#endif // CCACHE_H
