#include "object.h"
#include "zmalloc.h"
#include <stdio.h>

robj *objectCreate(int type, void *ptr) {
    robj *o = zmalloc(sizeof(*o));
    o->type = type;
    o->ptr = ptr;
    o->refcount = 1;

    /* Set the LRU to the current lruclock (minutes resolution).
     * We do this regardless of the fact VM is active as LRU is also
     * used for the maxmemory directive when Redis is used as cache.
     *
     * Note that this code may run in the context of an I/O thread
     * and accessing server.lruclock in theory is an error
     * (no locks). But in practice this is safe, and even if we read
     * garbage Redis will not fail. */
    o->lru = 1;
    return o;
}

robj *objectCreateNULL() {
    return objectCreate(OBJ_NULL,NULL);
}

robj *objFromSds(sds ptr) {
    return objectCreate(OBJ_SDS,ptr);
}


robj *objFromChar(char *ptr, size_t len) {
    return objectCreate(OBJ_SDS,sdsnewlen(ptr,len));
}

robj *objectDupSds(robj *o) {
    return objFromChar(o->ptr,sdslen(o->ptr));
}


void objectIncrRef(robj *o) {
    o->refcount++;
}

void objectDecrRef(void *obj) {
    robj *o = obj;

    if (o->refcount <= 0) printf("decrRefCount against refcount <= 0\n");
    if (o->refcount == 1) {
        switch(o->type) {
        case OBJ_SDS: sdsfree(o->ptr);; break;
        default: printf("Unknown object type\n"); break;
        }
        zfree(o);
    } else {
        o->refcount--;
    }
}

int checkType(robj *o, int type) {
    return o->type == type;
}

/* Compare two string objects via strcmp() or alike.
 * Note that the objects may be integer-encoded. In such a case we
 * use ll2string() to get a string representation of the numbers on the stack
 * and compare the strings, it's much faster than calling getDecodedObject().
 *
 * Important note: if objects are not integer encoded, but binary-safe strings,
 * sdscmp() from sds.c will apply memcmp() so this function ca be considered
 * binary safe. */
int objectCompareSds(robj *a, robj *b) {
    assert(a->type == OBJ_SDS && b->type == OBJ_SDS);
    return sdscmp((sds)a->ptr,(sds)b->ptr);
}

size_t objectSdsLen(robj *o) {
    assert(o->type == OBJ_SDS);
    return sdslen(o->ptr);
}
/* Given an object returns the min number of seconds the object was never
 * requested, using an approximated LRU algorithm. */
/*
unsigned long estimateObjectIdleTime(robj *o) {
    if (server.lruclock >= o->lru) {
        return (server.lruclock - o->lru) * CCACHE_LRU_CLOCK_RESOLUTION;
    } else {
        return ((CCACHE_LRU_CLOCK_MAX - o->lru) + server.lruclock) *
                    CCACHE_LRU_CLOCK_RESOLUTION;
    }
}
*/
