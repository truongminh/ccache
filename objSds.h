#ifndef OBJSDS_H
#define OBJSDS_H
#include <malloc.h>
#include <assert.h>
#include "sds.h"

typedef struct {
    sds ptr;
    int ref;
} objSds;

objSds *objSdsCreate();
objSds *objSdsFromSds(sds ptr);
void objSdsAddRef(objSds *obj);
void objSdsSubRef(objSds *obj);
#endif // OBJSDS_H
