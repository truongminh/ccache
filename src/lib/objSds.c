/* objSds.c - pseudo-object with state
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
