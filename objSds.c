#include "objSds.h"

objSds *objSdsCreate(){
    objSds *obj = malloc((sizeof(*obj)));
    obj->ptr = NULL;
    obj->ref = 1;
    return obj;
}

objSds *objSdsFromSds(sds ptr){
    objSds *obj = malloc((sizeof(*obj)));
    obj->ptr = ptr;
    obj->ref = 1;
    return obj;
}

void objSdsAddRef(objSds *obj){
    obj->ref++;
}

void objSdsSubRef(objSds *obj){
    assert(obj->ref);
    obj->ref--;
    if(obj->ref == 0) {
        sdsfree(obj->ptr);
        free(obj);
    }
}
