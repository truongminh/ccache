/* objSds.h
 */

#ifndef OBJSDS_H
#define OBJSDS_H
#include <malloc.h>
#include <assert.h>
#include "ccache_config.h"
#include "sds.h"
#include "adlist.h"

#define OBJSDS_WAITING 0
#define OBJSDS_OK 1
#define OBJSDS_ERR 2

typedef struct {
    int state;
    sds ptr;
    int ref;
    list *waiting_entries;
} objSds;

objSds *objSdsCreate();
void objSdsAddWaitingEntry(objSds *obj, void* entry);
objSds *objSdsFromSds(sds ptr);
void objSdsAddRef(objSds *obj);
void objSdsSubRef(objSds *obj);

#if(CCACHE_LOG_LEVEL == CCACHE_DEBUG)
    #define OBJ_REPORT_REF(obj) printf("OBJECT %p REF %d \n",obj,obj->ref)
#else
    #define OBJ_REPORT_REF(obj) ;
#endif

#endif // OBJSDS_H
