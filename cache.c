/*
  I/O: sort request by block to reduce hard disk seek time.

*/

#include <pthread.h>
#include <unistd.h>
#include "cache.h"
#include "malloc.h"
#include "string.h" /* for memcpy */
#include "dicttype.h"
#include "objSds.h"
#include "ufile.h"

/*
* long page_size = sysconf (_SC_PAGESIZE);
*/

#ifdef CCACHE_DEBUG
    #define REPORT_MASTER_ADD_KEY(key) printf("Master \t Add new entry [%s]\n",key)
#else
    #define REPORT_MASTER_ADD_KEY(key) ; /* just ignore */
#endif

static pthread_t master_thread;
static dict *master_cache;
static list *slave_caches;
static void *_masterWatch(void *t);
objSds *HTTP_NOT_FOUND;
void _masterUnwatchClient(safeQueue* watching_clients, sds obuf);
void _masterProcessCacheNew(ccache *c);
void _masterProcessCacheOld(ccache *c);
void _masterProcessFinishedIO();

void cacheMasterInit() {
    pthread_attr_t attr;
    HTTP_NOT_FOUND = objSdsFromSds(sdsnew("HTTP/1.1 200 OK\r\nContent-Length: 9\r\n\r\nNot Found"));
    objSdsAddRef(HTTP_NOT_FOUND);
    /* Initialize mutex and condition variable objects */    
    /* For portability, explicitly create threads in a joinable state */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    master_cache = dictCreate(&objSdsDictType,NULL);
    slave_caches = listCreate();
    pthread_create(&master_thread, &attr, _masterWatch, NULL);
    bioInit();
}

ccache *cacheAddSlave(void *el) {
    ccache *c = cacheCreate();
    c->el = el;
    listAddNodeTail(slave_caches,c);
    return c;
}

/*
  The master works as a Complete-Fair Queuing (CFQ) I/O Scheduler.
  The master visits each queue, servicing each request until
   the queue's timeslice is exhausted, or until no more requests remain.
  NOTE: current implemenetation does not check for a queue's timeslice
*/

void *_masterWatch(void *t)
{
    (void)t;
    ccache *c;
    listIter li;
    listNode *ln;
    while (1) {        
        listRewind(slave_caches,&li);
        while ((ln = listNext(&li)) != NULL) {
            c = listNodeValue(ln);
            _masterProcessCacheNew(c);
            _masterProcessFinishedIO();
            _masterProcessCacheOld(c);
        }
        usleep(100);
    }
    pthread_exit(NULL);
}

void _masterUnwatchClient(safeQueue* watching_clients, sds obuf) {
    httpClient *client;
    while((client = safeQueuePop(watching_clients)) != NULL) {
        unblockClient(client,obuf);
        /* */
    }
}

void _masterProcessCacheNew(ccache *c){
    cacheEntry *ce;
    while((ce=cacheGetMessage(c,CACHE_NEW)) != NULL)
    {
        sds key = ce->de->key;
        objSds *value = dictFetchValue(master_cache,key);
        if(!value) {
            REPORT_MASTER_ADD_KEY(key);
            value = objSdsCreate();
            /* Add cache entry to waiting list */
            objSdsAddWaitingEntry(value,ce);
            /* Add entry to master cache */
            sds mkey = sdsdup(key); /* master must have its own key for its own cache */
            /* Every when accept new ce, the obj ref is increased */
            objSdsAddRef(value);
            dictAdd(master_cache,mkey,value);
            /* New IO Job */
            bioCreateBackgroundJob(0,mkey);
            OBJ_REPORT_REF(value);
        }
        else {
            switch(value->state) {
            case OBJSDS_WAITING:
                /* Every when accept new ce, the obj ref is increased */
                objSdsAddRef(value);
                objSdsAddWaitingEntry(value,ce);
                break;
            case OBJSDS_OK:
                ce->val = value->ptr;
                /* Every when accept new ce, the obj ref is increased */
                objSdsAddRef(value);
                /* Unblock waiting clients */
                _masterUnwatchClient(ce->waiting_clients, ce->val);
                break;
            default:
                /* Error Unknown Object State */
                break;
            }
            OBJ_REPORT_REF(value);
        }
    }
}

void _masterProcessFinishedIO() {
    sds key = NULL;
    sds content = NULL;
    /* For each IO worker */
    int tid = 0;
    for(tid=0;tid<CCACHE_NUM_IO_THREADS;tid++) {
        while(bioGetResult(tid,&key,&content))
        {
            objSds *value = dictFetchValue(master_cache,key);
            if(content == NULL) value->ptr = HTTP_NOT_FOUND->ptr;
            else value->ptr = content;
            value->state = OBJSDS_OK;
            listIter li;
            listNode *ln;
            cacheEntry *ce;
            listRewind(value->waiting_entries,&li);
            /* for each waiting ce */
            while ((ln = listNext(&li)) != NULL){
                /* unwatch client */
                ce = listNodeValue(ln);
                ce->val = value->ptr;
                _masterUnwatchClient(ce->waiting_clients, ce->val);
            }
        }
        }
}

void _masterProcessCacheOld(ccache *c){
    sds old_key;
    while((old_key=cacheGetMessage(c,CACHE_OLD)) != NULL)
    {
        objSds *value = dictFetchValue(master_cache,old_key);
        if(value) {
            objSdsSubRef(value);
            OBJ_REPORT_REF(value);
            if(value->ref == 1) dictDelete(master_cache,old_key);
        }
        sdsfree(old_key);
    }
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
    c->forOld = safeQueueCreate();
    c->forNew = safeQueueCreate();
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
    ce->waiting_clients = safeQueueCreate();
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
    /* Master reply by setting val to an object.
     * We do not delete cache entry until the master reply
     */
    if(ce&&ce->val)
    {
        cacheSendMessage(c,sdsdup(key),CACHE_OLD);
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

int cacheSendMessage(ccache *c, void *ce, int forWhom){
    if(forWhom == CACHE_NEW)
        return (safeQueuePush(c->forNew,ce) == SAFE_QUEUE_OK)? CACHE_OK:CACHE_ERR;
    if(forWhom == CACHE_OLD)
        return (safeQueuePush(c->forOld,ce) == SAFE_QUEUE_OK)? CACHE_OK:CACHE_ERR;
    return CACHE_ERR;
}

void *cacheGetMessage(ccache *c, int forWhom){
    if(forWhom == CACHE_NEW)
        return safeQueuePop(c->forNew);
    if(forWhom== CACHE_OLD)
        return safeQueuePop(c->forOld);
    return NULL;
}

void cacheAddWatchClient(cacheEntry *ce, httpClient *client) {
    safeQueuePush(ce->waiting_clients,client);
}
