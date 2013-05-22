#include "object.h"
#include "zmalloc.h"
#include <stdio.h>

robj *createObject(int type, void *ptr) {
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

robj *createStringObject(char *ptr, size_t len) {
    return createObject(REDIS_STRING,sdsnewlen(ptr,len));
}

robj *dupStringObject(robj *o) {
    return createStringObject(o->ptr,sdslen(o->ptr));
}


void incrRefCount(robj *o) {
    o->refcount++;
}

void decrRefCount(void *obj) {
    robj *o = obj;

    if (o->refcount <= 0) printf("decrRefCount against refcount <= 0\n");
    if (o->refcount == 1) {
        switch(o->type) {
        case REDIS_STRING: sdsfree(o->ptr);; break;
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
int compareStringObjects(robj *a, robj *b) {
    assert(a->type == REDIS_STRING && b->type == REDIS_STRING);
    return sdscmp((sds)a->ptr,(sds)b->ptr);
}

size_t stringObjectLen(robj *o) {
    assert(o->type == REDIS_STRING);
    return sdslen(o->ptr);
}
/* Given an object returns the min number of seconds the object was never
 * requested, using an approximated LRU algorithm. */
/*
unsigned long estimateObjectIdleTime(robj *o) {
    if (server.lruclock >= o->lru) {
        return (server.lruclock - o->lru) * REDIS_LRU_CLOCK_RESOLUTION;
    } else {
        return ((REDIS_LRU_CLOCK_MAX - o->lru) + server.lruclock) *
                    REDIS_LRU_CLOCK_RESOLUTION;
    }
}
*/
