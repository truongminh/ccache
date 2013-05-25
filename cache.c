#include "cache.h"
#include "malloc.h"
#include "string.h" /* for memcpy */
#include "dicttype.h"


void dictCacheEntryDestructor(void *privdata, void *val)
{
    DICT_NOTUSED(privdata);
    cacheEntry *ce = (cacheEntry*)val;
    ce->de = NULL;
    decrRefCount(ce->value);
    free(ce);
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
    list *accesslist = listCreate();
    /* Free Node Value */
    return dictCreate(&ccacheType,accesslist);
}


cacheEntry *cacheAdd(dict *c, char* key, robj *value) {
    cacheEntry *ce;
    if ((ce = malloc(sizeof(*ce))) == NULL)
        return NULL;
    ce->value = value;
    incrRefCount(value);
    sds dk = sdsnew(key);
    if((ce->de = dictAddGetEntry(c,dk,ce))) {
        list *accessList = (list*)c->privdata;
        ce->ln = listAddNodeTailGetNode(accessList,dk);
    }
    return NULL;
}

cacheEntry *cacheFind(dict* c, sds key) {
    cacheEntry *ce = (cacheEntry *)dictFetchValue(c,sdsnew(key));
    list *accessList = (list*)c->privdata;
    if(ce) listMoveNodeToTail(accessList,ce->ln);
    return ce;
}

void cacheDelete(ccache* c, char* key) {
    dictDelete(c,key);
}

int cacheDeleteStaleEntries(ccache *c, unsigned int n) {
    list *accesslist  = cacheGetList(c);
    listNode *node = accesslist->head;
    unsigned int remain = n;
    while(remain--&&node) {
        if (node->prev)
            node->prev->next = node->next;
        else
            accesslist->head = node->next;
        if (node->next)
            node->next->prev = node->prev;
        else
            accesslist->tail = node->prev;
        /* Free cache Entry */
        dictDelete(c,(sds)(node->value));
        free(node);
        accesslist->len--;
        node = listNextNode(node); /* usage of node is unsafe */
    }
    /* Return the number of successfully deleted entries */
    return n - remain;
}

