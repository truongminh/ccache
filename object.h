#ifndef OBJECT_H
#define OBJECT_H
#include "sds.h"
#include "assert.h"

#define REDIS_STRING 1
typedef struct redisObject {
    unsigned type:4;
    unsigned storage:2;     /* REDIS_VM_MEMORY or REDIS_VM_SWAPPING */
    unsigned encoding:4;
    unsigned lru:22;        /* lru time (relative to server.lruclock) */
    int refcount;
    void *ptr;
    /* VM fields are only allocated if VM is active, otherwise the
     * object allocation function will just allocate
     * sizeof(redisObjct) minus sizeof(redisObjectVM), so using
     * Redis without VM active will not have any overhead. */
} robj;


robj *createObject(int type, void *ptr) ;
robj *createStringObject(char *ptr, size_t len) ;

robj *dupStringObject(robj *o) ;


void incrRefCount(robj *o) ;
void decrRefCount(void *obj);
int checkType(robj *o, int type) ;

/* Compare two string objects via strcmp() or alike.
 * Note that the objects may be integer-encoded. In such a case we
 * use ll2string() to get a string representation of the numbers on the stack
 * and compare the strings, it's much faster than calling getDecodedObject().
 *
 * Important note: if objects are not integer encoded, but binary-safe strings,
 * sdscmp() from sds.c will apply memcmp() so this function ca be considered
 * binary safe. */
int compareStringObjects(robj *a, robj *b);

size_t stringObjectLen(robj *o);

#endif // OBJECT_H
