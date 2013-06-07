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

/*
* long page_size = sysconf (_SC_PAGESIZE);
*/

static pthread_mutex_t count_mutex;
static pthread_mutex_t mutex_gcache;
static pthread_cond_t cond_newkey;
static pthread_t master_thread;
static dict *master_cache;
static list *slave_caches;
static void *watch_cache(void *t);

void unwatch_client(void *el, safeQueue* watching_clients, sds obuf);

void cacheMasterInit() {
    pthread_attr_t attr;

    /* Initialize mutex and condition variable objects */
    pthread_mutex_init(&count_mutex, NULL);
    pthread_mutex_init(&mutex_gcache, NULL);
    pthread_cond_init (&cond_newkey, NULL);

    /* For portability, explicitly create threads in a joinable state */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    master_cache = dictCreate(&objSdsDictType,NULL);
    slave_caches = listCreate();
    pthread_create(&master_thread, &attr, watch_cache, NULL);
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

void *watch_cache(void *t)
{
    (void)t;
    ccache *c;
    cacheEntry *ce;
    sds okey;
    objSds *notfound = objSdsFromSds(sdsnew("HTTP/1.1 200 OK\r\nContent-Length: 9\r\n\r\nNot Found"));;
    objSdsAddRef(notfound);
    FILE *fp;
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
                    printf("Master \t Add new entry [%s]\n",key);
                    fp = fopen(key+1,"r");
                    if(fp == NULL) {
                        value = notfound;
                    }
                    else {
                        char BUF[101];
                        int nread = fread(BUF,1,100,fp);
                        sds content = sdsempty();
                        content = sdscatprintf(content,"HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n",nread);
                        content = sdscatlen(content,BUF,nread);
                        value = objSdsFromSds(content);
                        fclose(fp);
                    }
                    dictAdd(master_cache,sdsdup(key),value);
                }
                ce->val = value->ptr;
                objSdsAddRef(value);
                printf("OBJECT %p REF %d \n",value,value->ref);
                /* Unblock waiting clients */
                unwatch_client(c->el,ce->waiting_clients, ce->val);
            }
            while((okey=cacheGetMessage(c,CACHE_OLD)) != NULL)
            {                
                objSds *value = dictFetchValue(master_cache,okey);
                if(value) {
                    objSdsSubRef(value);
                    printf("OBJECT %p REF %d \n",value,value->ref);
                    if(value->ref == 1) dictDelete(master_cache,okey);
                }
                sdsfree(okey);
            }
        }
        usleep(100);
    }
    pthread_exit(NULL);
}

void unwatch_client(void *el, safeQueue* watching_clients, sds obuf) {
    httpClient *client;
    while((client = safeQueuePop(watching_clients)) != NULL) {
        unblockClient(el,client,obuf);
        /* */
    }
}

void unwatchClientNotFound(void *el, safeQueue* watching_clients) {
    httpClient *client;
    while((client = safeQueuePop(watching_clients)) != NULL) {
        unblockClientNotFound(el,client);
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
