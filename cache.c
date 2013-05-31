
#include <pthread.h>
#include <unistd.h>
#include "cache.h"
#include "malloc.h"
#include "string.h" /* for memcpy */
#include "dicttype.h"
#include "objSds.h"

static pthread_mutex_t count_mutex;
static pthread_mutex_t mutex_gcache;
static pthread_cond_t cond_newkey;
static pthread_t master_thread;
static dict *master_cache;
static list *slave_caches;
static void *watch_cache(void *t);

void unwatch_client(void *el, safeList* watching_clients, sds obuf);

void cacheMasterInit() {
    pthread_attr_t attr;

    /* Initialize mutex and condition variable objects */
    pthread_mutex_init(&count_mutex, NULL);
    pthread_mutex_init(&mutex_gcache, NULL);
    pthread_cond_init (&cond_newkey, NULL);

    /* For portability, explicitly create threads in a joinable state */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    master_cache = dictCreate(&sdsDictType,NULL);
    slave_caches = listCreate();
    pthread_create(&master_thread, &attr, watch_cache, NULL);
}

ccache *cacheAddSlave(void *el) {
    ccache *c = cacheCreate();
    c->el = el;
    listAddNodeTail(slave_caches,c);
    return c;
}

void *watch_cache(void *t)
{
    (void)t;
    ccache *c;
    cacheEntry *ce;
    while (1) {
        listIter li;
        listNode *ln;
        listRewind(slave_caches,&li);
        while ((ln = listNext(&li)) != NULL) {
            c = listNodeValue(ln);
            while((ce=cacheGetMessage(c,CACHE_NEW)) != NULL)
            {
                sds key = ce->de->key;
                objSds *value = dictFetchValue(master_cache,key);
                if(!value) {
                    printf("Master \t Add new entry\n");
                    value = objSdsFromSds(sdsnew("HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK"));
                    dictAdd(master_cache,sdsdup(key),value);
                }
                ce->val = value->ptr;
                objSdsAddRef(value);
                printf("OBJECT %p REF %d \n",value,value->ref);
                /* Unblock waiting clients */
                unwatch_client(c->el,ce->waiting_clients, ce->val);
            }
            while((ce=cacheGetMessage(c,CACHE_OLD)) != NULL)
            {
                sds key = ce->de->key;
                objSds *value = dictFetchValue(master_cache,key);
                if(value) {
                    objSdsSubRef(value);
                    printf("OBJECT %p REF %d \n",value,value->ref);
                    if(value->ref == 1) dictDelete(master_cache,key);
                }
            }
        }
        usleep(100);
    }
    pthread_exit(NULL);
}

void unwatch_client(void *el, safeList* watching_clients, sds obuf) {
    httpClient *client;
    while((client = safeListGetHead(watching_clients)) != NULL) {
        unblockClient(el,client,obuf);
        /* */
    }
}

void dictCacheEntryDestructor(void *privdata, void *val)
{
    DICT_NOTUSED(privdata);
    cacheEntry *ce = (cacheEntry*)val;
    free(ce->de->val);
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
    ccache *c = malloc(sizeof(*c));
    c->accesslist = listCreate();
    c->data = dictCreate(&ccacheType,NULL);
    c->forOld = safeListCreate();
    c->forNew = safeListCreate();
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
    ce->waiting_clients = safeListCreate();
    ce->de->val = ce;
    ce->val = value;
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
        cacheSendMessage(c,ce,CACHE_NEW);
    }
    return ce;
}

void cacheDelete(ccache* c, sds key) {
    cacheEntry *ce = (cacheEntry *)dictFetchValue(c->data,key);
    listDelNode(c->accesslist,ce->ln);
    dictDelete(c->data,key);
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
        dictDelete(c->data,(sds)(node->value));
        free(node);
        accesslist->len--;
        node = listNextNode(node); /* usage of node is unsafe */
    }
    /* Return the number of successfully deleted entries */
    return n - remain;
}

int cacheSendMessage(ccache *c, cacheEntry *ce, int forWhom){
    if(forWhom == CACHE_NEW)
        return (safeListAddNodeTail(c->forNew,ce) == SAFE_LIST_OK)? CACHE_OK:CACHE_ERR;
    if(forWhom == CACHE_OLD)
        return (safeListAddNodeTail(c->forOld,ce) == SAFE_LIST_OK)? CACHE_OK:CACHE_ERR;
    return CACHE_ERR;
}

cacheEntry *cacheGetMessage(ccache *c, int forWhom){
    if(forWhom == CACHE_NEW)
        return safeListGetHead(c->forNew);
    if(forWhom== CACHE_OLD)
        return safeListGetHead(c->forOld);
    return NULL;
}

void cacheAddWatchClient(cacheEntry *ce, httpClient *client) {
    safeListAddNodeTail(ce->waiting_clients,client);
}
