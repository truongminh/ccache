/* objSds.h
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
