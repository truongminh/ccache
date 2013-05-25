#ifndef CCACHE_H
#define CCACHE_H

#include "dict.h"
#include "adlist.h"
#include "object.h"

typedef struct cacheEntry {
    dictEntry *de;
    listNode *ln;
    robj* value;
} cacheEntry;

typedef dict ccache;

#define cacheGetList(c) ((list*)c->privdata)
ccache *cacheCreate();
int cacheDeleteStaleEntries(ccache *c, unsigned int n);
void cacheDelete(ccache* c, char* key);
cacheEntry *cacheAdd(dict *ccache, char* key, robj *value);
cacheEntry *cacheFind(dict* ccache, sds key);
#define cacheNumberOfEntry(c) c->used

#endif // CCACHE_H
