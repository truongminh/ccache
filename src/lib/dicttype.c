/* dictype.c - Frequently used dicttype
 */

#include <malloc.h> /* define NULL value */
#include <string.h> /* memcpy */
#include "dicttype.h"
#include "adlist.h"
#include "sds.h"
#include "objSds.h"

int dictSdsKeyCompare(void *privdata, const void *key1,
        const void *key2)
{
    int l1,l2;
    DICT_NOTUSED(privdata);

    l1 = sdslen((sds)key1);
    l2 = sdslen((sds)key2);
    if (l1 != l2) return 0;
    return memcmp(key1, key2, l1) == 0;
}

void dictSdsDestructor(void *privdata, void *val)
{
    DICT_NOTUSED(privdata);

    sdsfree(val);
}


void dictGeneralDestructor(void *privdata, void *val)
{
    DICT_NOTUSED(privdata);
    free(val);
}

void dictListDestructor(void *privdata, void *val)
{
    DICT_NOTUSED(privdata);

    listRelease(val);
}

void dictObjSdsDestructor(void *privdata, void *val)
{
    DICT_NOTUSED(privdata);
    objSdsSubRef((objSds*)val);
}

unsigned int dictSdsHash(const void *key) {
    return dictGenHashFunction((unsigned char*)key, sdslen((char*)key));
}

dictType sdsDictType = {
    dictSdsHash,                /* hash function */
    NULL,                       /* key dup */
    NULL,                       /* val dup */
    dictSdsKeyCompare,          /* key compare */
    dictSdsDestructor,          /* key destructor */
    dictSdsDestructor           /* val destructor */
};

dictType sdsDoubleDictType = {
    dictSdsHash,                /* hash function */
    NULL,                       /* key dup */
    NULL,                       /* val dup */
    dictSdsKeyCompare,          /* key compare */
    dictSdsDestructor,          /* key destructor */
    dictGeneralDestructor           /* val destructor */
};

dictType keylistDictType = {
    dictSdsHash,                /* hash function */
    NULL,                       /* key dup */
    NULL,                       /* val dup */
    dictSdsKeyCompare,          /* key compare */
    dictSdsDestructor,  /* key destructor */
    dictListDestructor          /* val destructor */
};


dictType objSdsDictType = {
    dictSdsHash,                /* hash function */
    NULL,                       /* key dup */
    NULL,                       /* val dup */
    dictSdsKeyCompare,          /* key compare */
    dictSdsDestructor,          /* key destructor */
    dictObjSdsDestructor/* val destructor */
};
