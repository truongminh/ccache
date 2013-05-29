
#include <pthread.h>
#include "cache.h"
#include "malloc.h"
#include "string.h" /* for memcpy */
#include "dicttype.h"

static pthread_mutex_t count_mutex;
static pthread_mutex_t master_mutex;
static pthread_cond_t cond_newkey;
static pthread_t master_thread;
static dict* waiting_keys;

static void *watch_cache(void *t);

void cacheMasterInit() {
    pthread_attr_t attr;

    /* Initialize mutex and condition variable objects */
    pthread_mutex_init(&count_mutex, NULL);
    pthread_mutex_init(&master_mutex, NULL);
    pthread_cond_init (&cond_newkey, NULL);

    /* For portability, explicitly create threads in a joinable state */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    waiting_keys = dictCreate(&keylistDictType,NULL);
    pthread_create(&master_thread, &attr, watch_cache, NULL);
}

void *watch_cache(void *t)
{
  /*
  Lock mutex and wait for signal.  Note that the pthread_cond_wait
  routine will automatically and atomically unlock mutex while it waits.
  Also, note that if COUNT_LIMIT is reached before this routine is run by
  the waiting thread, the loop will be skipped to prevent pthread_cond_wait
  from never returning.
  */
    DICT_NOTUSED(t);
  pthread_mutex_lock(&count_mutex);
  while (1) {
    pthread_cond_wait(&cond_newkey, &count_mutex);
    printf("Add Dict Entry \n");
   }
  pthread_mutex_unlock(&count_mutex);
  pthread_exit(NULL);
}


void cacheAddWaitingKeys(sds key, void* c){
    /* And in the other "side", to map keys -> clients */
    dictEntry *de = dictFind(waiting_keys,key);
    list *l;
    if (de == NULL) {
        int retval;

        /* For every key we take a list of clients blocked for it */
        l = listCreate();
        retval = dictAdd(waiting_keys,key,l);
        // incrRefCount(key);
    } else {
        l = dictGetEntryVal(de);
    }
    pthread_mutex_lock(&count_mutex);
    listAddNodeTail(l,c);
    pthread_cond_signal(&cond_newkey);
    pthread_mutex_unlock(&count_mutex);
}

void dictCacheEntryDestructor(void *privdata, void *val)
{
    DICT_NOTUSED(privdata);
    cacheEntry *ce = (cacheEntry*)val;
    objectDecrRef(ce->value);
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
    c->d = dictCreate(&ccacheType,NULL);
    c->in = safeListCreate();
    c->out = safeListCreate();
    return c;
}



int cacheAddEntry(ccache *c, sds key) {
    cacheEntry *ce;
    if ((ce = malloc(sizeof(*ce))) == NULL)
        return CACHE_ERR;
    ce->value = objectCreateNULL();
    if(dictAdd(c->d,key,ce) == DICT_OK) {
        ce->ln = listAddNodeTailGetNode(c->accesslist,key);
        return CACHE_OK;
    }
    return CACHE_ERR;
}

int cacheAdd(ccache *c, sds key, robj *value) {
    cacheEntry *ce;
    if ((ce = malloc(sizeof(*ce))) == NULL)
        return CACHE_ERR;
    ce->value = value;
    objectIncrRef(value);
    if(dictAdd(c->d,key,ce) == DICT_OK) {
        ce->ln = listAddNodeTailGetNode(c->accesslist,key);
        return CACHE_OK;
    }
    return CACHE_ERR;
}

robj *cacheFind(ccache *c, sds key) {
    cacheEntry *ce = (cacheEntry *)dictFetchValue(c->d,key);
    if(ce) {
        listMoveNodeToTail(c->accesslist,ce->ln);
        return ce->value;
    }
    return NULL;
}

void cacheDelete(ccache* c, sds key) {
    cacheEntry *ce = (cacheEntry *)dictFetchValue(c->d,key);
    listDelNode(c->accesslist,ce->ln);
    dictDelete(c->d,key);
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
        dictDelete(c->d,(sds)(node->value));
        free(node);
        accesslist->len--;
        node = listNextNode(node); /* usage of node is unsafe */
    }
    /* Return the number of successfully deleted entries */
    return n - remain;
}

int cacheSendMessage(ccache *c, robj *obj){
    return (safeListAddNodeTail(c->out,obj) == SAFE_LIST_OK)? CACHE_OK:CACHE_ERR;
}

int cacheRequest(ccache *c, sds key){
    cacheMsg *msg = malloc(sizeof(*msg));
    msg->key = sdsdup(key);
    msg->obj = NULL;
    return (safeListAddNodeTail(c->out,msg) == SAFE_LIST_OK)? CACHE_OK:CACHE_ERR;
}

cacheMsg *cacheGetMessage(ccache *c){
    return safeListGetHead(c->in);
}
