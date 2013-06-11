#include "objSds.h"

objSds *objSdsCreate(){
    objSds *obj = malloc((sizeof(*obj)));
    obj->state = OBJSDS_WAITING;
    obj->ptr = NULL;
    obj->ref = 1;
    obj->waiting_entries = listCreate();
    return obj;
}

objSds *objSdsFromSds(sds ptr){
    objSds *obj = malloc((sizeof(*obj)));
    obj->ptr = ptr;
    obj->ref = 1;
    obj->waiting_entries = listCreate();
    return obj;
}

void objSdsAddWaitingEntry(objSds *obj, void* entry){
    listAddNodeTail(obj->waiting_entries,entry);
}

void objSdsAddRef(objSds *obj){
    obj->ref++;
}

void objSdsSubRef(objSds *obj){
    assert(obj->ref);
    obj->ref--;
    if(obj->ref == 0) {
        sdsfree(obj->ptr);
        listRelease(obj->waiting_entries);
        free(obj);
    }
}
