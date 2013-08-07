/* mcache.c - master cache implementation
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

#include <pthread.h>
#include "mcache.h"
#include "sds.h"
#include "dict.h"
#include "dicttype.h"
#include "adlist.h"
#include "objSds.h"
#include "bio.h"
#include "safe_queue.h"
#include "cache.h"


static pthread_t master_thread;
static dict *master_cache = NULL;
static dict *file_sizes = NULL;
static double master_total_mem = 0;
static double master_total_disk = 0;
static void *_masterWatch(void *t);
struct FileInfo {
    sds fn;
    size_t size;
};
void freeFileInfo(void *ptr)
{
    if(ptr) {
        struct FileInfo *fi = (struct FileInfo*)ptr;
        sdsfree(fi->fn);
        free(fi);
    }
}

static list *tmpCreated;
static objSds *HTTP_NOT_FOUND = NULL;
static sds statusQuery;
static unsigned long next_master_refresh_time = 0;

static void _masterUnwatchClient(safeQueue* watching_clients, sds obuf);
static void _masterProcessCacheNew(ccache *c);
static void _masterProcessCacheOld(ccache *c);
static void _masterProcessFinishedIO();
static void _masterProcessStatus();
static sds _masterGetStatus();

void cacheMasterInit() {
    pthread_attr_t attr;
    tmpCreated = listCreate();
    listSetFreeMethod(tmpCreated,freeFileInfo);
    master_cache = dictCreate(&objSdsDictType,NULL);
    slave_caches = listCreate();
    file_sizes = dictCreate(&sdsDoubleDictType,NULL);
    master_total_mem = 0;
    master_total_disk = 0;
    HTTP_NOT_FOUND = objSdsFromSds(sdsnew("HTTP/1.1 404 OK\r\nContent-Length: 9\r\n\r\nNot Found"));
    objSdsAddRef(HTTP_NOT_FOUND);
    statusQuery = sdsnew("/status");
    objSds *value = objSdsCreate();
    value->ref = 2; /* ensure that '/status' entry will not be freed */
    next_master_refresh_time += time(NULL) + MASTER_STATUS_REFRESH_PERIOD;
    value->state = OBJSDS_OK;
    dictAdd(master_cache,statusQuery,value);
    value->ptr = _masterGetStatus();
    /* Initialize mutex and condition variable objects */
    /* For portability, explicitly create threads in a joinable state */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&master_thread, &attr, _masterWatch, NULL);
    bioInit();
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
            _masterProcessStatus();
        }
        /* TODO: flexible sleep time */
        usleep(1000);
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
            bioPushGeneralJob(mkey);
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
    int type = 0;
    /* Polling all io thread */
    for(tid=0;tid<CCACHE_NUM_BIO_THREADS;tid++) {
        while((type=bioGetResult(tid,&key,&content)))
        {
            objSds *value = dictFetchValue(master_cache,key);
            if(content == NULL) {
                value->ptr = HTTP_NOT_FOUND->ptr;
            }
            else {
                value->ptr = content;
                master_total_mem += sdslen(content);
            }
            if(type&BIO_WRITE_FILE) {
                struct FileInfo *fi = (struct FileInfo*)malloc(sizeof(struct FileInfo));
                fi->fn = sdsdup(key);
                fi->size = sdslen(content);
                /* if the key already exists, dictAdd does nothing. */
                listAddNodeTail(tmpCreated,fi);
                master_total_disk += sdslen(content);
            }
            value->state = OBJSDS_OK;
            listIter li;
            listNode *ln;
            cacheEntry *ce;
            /* notify all cache entries waiting for this key */
            listRewind(value->waiting_entries,&li);
            /* for each waiting ce */
            while ((ln = listNext(&li)) != NULL){
                /* unwatch client */
                ce = listNodeValue(ln);
                ce->val = value->ptr;
                /* notify all clients waiting for this entry */
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
            /* No ae thread use this entry anymore */
            if(value->ref == 1) {
                printf("mem freed\n");
                master_total_mem -= sdslen(value->ptr);
                /* TODO: send free mem task to background job threads */
                dictDelete(master_cache,old_key);
            }
        }
        sdsfree(old_key);
    }
}

void _masterProcessStatus() {
    /* Check if status is expired */
    unsigned long now = time(NULL);
    if(next_master_refresh_time<now) {
        objSds *value = dictFetchValue(master_cache,statusQuery);
        if(value) {
            sds oldptr = value->ptr;
            /* Re-asign the value */
            value->ptr = _masterGetStatus();
            sdsfree(oldptr);
            next_master_refresh_time = now + MASTER_STATUS_REFRESH_PERIOD;
        }
        else {
            ulog(CCACHE_WARNING,"master cache %s not found",statusQuery);
            next_master_refresh_time = now + MASTER_STATUS_REFRESH_PERIOD*1000;
        }
    }    
    listIter li;
    listNode *ln;
    listRewind(tmpCreated,&li);
    while ((ln = listNext(&li)) != NULL && master_total_disk > MASTER_MAX_AVAIL_DISK) {
        struct FileInfo *fi = listNodeValue(ln);
        bioPushRemoveFileJob(fi->fn);
        master_total_disk -= fi->size;
        listDelNode(tmpCreated,ln);
    }
}

sds _masterGetStatus() {
    /*TODO: calculate cache increase speed,
     * then adopt a suitable stale-cache freeing strategy
     * Three involved params:
     * (1) master sleep,
     * (2) ae loop wait,
     * and (3) number of stale entries in one ae loop
     */

    sds status = sdsempty();//sdsfromlonglong(master_total_mem);
    status = sdscatprintf(status,"TOL RAM: %-6.2lfMB\tUSED RAM: %-6.2lf\n",
                          BYTES_TO_MEGABYTES(MASTER_MAX_AVAIL_MEM),
                          BYTES_TO_MEGABYTES(master_total_mem));
    status = sdscatprintf(status,"TOL DISK: %-6.2lfMB\tUSED DISK: %-6.2lf\n",
                          BYTES_TO_MEGABYTES(MASTER_MAX_AVAIL_DISK),
                          BYTES_TO_MEGABYTES(master_total_disk));
    status = sdscatprintf(status,"Detail:\n");
    status = sdscatprintf(status,"%-3s %-32s: %-6s\n"," ","KEY","MEM");
    dictIterator *di = dictGetIterator(master_cache);
    dictEntry *de;
    int idx = 1;
    while((de = dictNext(di)) != NULL) {
        objSds *value = (objSds*)dictGetEntryVal(de);
        if(value) {
            if(value->ptr) {
                status = sdscatprintf(status,"%-3d %-32s: %-6ld\n",
                                        idx++,
                                        (char*)dictGetEntryKey(de),
                                        sdslen(value->ptr));
            }
            else {
                status = sdscatprintf(status,"%-3d %-32s: %-6s\n",
                                        idx++,
                                        (char*)dictGetEntryKey(de),
                                        "WAITING");
            }
        }
    }
    dictReleaseIterator(di);
    sds status_reply = sdsnew("HTTP/1.1 200 OK\r\n");
    status_reply = sdscatprintf(status_reply,"Content-Length: %ld\r\n\r\n%s",sdslen(status),status);
    sdsfree(status);
    return status_reply;
}

int shouldIFreeSomeData(){
    return master_total_mem > MASTER_MAX_AVAIL_MEM;
}
