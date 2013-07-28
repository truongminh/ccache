/* reply.c - http reply
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

#include "reply.h"
#include "malloc.h"
#include "dicttype.h"


reply* replyCreate() {
    reply* r;
    if((r = malloc(sizeof(*r))) == NULL) return NULL;
    r->status = reply_ok;
    r->headers = dictCreate(&sdsDictType,NULL);
    r->obuf = NULL;
    r->content = sdsempty();
    r->isCached = 0;
    return r;
}

void replyFree(reply* r) {
    dictRelease(r->headers);
    sdsfree(r->content);
    if(r->obuf&&r->isCached == 0) sdsfree(r->obuf);
    free(r);
}

void replyReset(reply *r) {
    dictRelease(r->headers);
    r->headers = dictCreate(&sdsDictType,NULL);
    sdsclear(r->content);
    if(r->isCached) r->obuf = NULL;
    else sdsfree(r->obuf);
}


sds replyToBuffer(reply* r) {
    if(r->obuf == NULL) {
        sds obuf = sdsempty();
        obuf = sdscat(obuf,replyStatusToString(r->status));
        dictIterator *di = dictGetIterator(r->headers);
        dictEntry* de;
        while((de = dictNext(di)) != NULL) {
            obuf = sdscatsds(obuf,dictGetEntryKey(de));
            obuf = sdscat(obuf,": ");
            obuf = sdscatsds(obuf,dictGetEntryVal(de));
            obuf = sdscat(obuf,"\r\n");
        }
        dictReleaseIterator(di);
        if(sdslen(r->content) > 0) {
            obuf = sdscatprintf(obuf,"Content-Length: %ld",sdslen(r->content));
            obuf = sdscat(obuf,"\r\n\r\n");
            obuf = sdscatsds(obuf,r->content);
        }
        r->obuf = obuf;
    }
    return r->obuf;
}

int replyAddHeader(reply *r, const char *name, const char *value) {
    return dictAdd(r->headers,sdsnew(name),sdsnew(value));
}

void replySetContent(reply *r , char* content){
    r->content = sdscat(r->content,content);
}

void replySetStatus(reply *r , reply_status_type status){

    r->status = status;
}

reply *replyStock(reply_status_type status) {
    reply* r = replyCreate();
    sds obuf = r->obuf;
    obuf = sdscatsds(obuf,replyStatusToString(status));
    obuf = sdscat(obuf,"Content-Length: 0");
    r->obuf = obuf;
    return r;
}


char* replyStatusToString(reply_status_type status)
{
    switch(status) {
    case reply_ok: return "HTTP/1.1 200 OK\r\n"; break;
    case reply_created: return "HTTP/1.1 201 Created\r\n"; break;
    case reply_accepted: return "HTTP/1.1 202 Accepted\r\n"; break;
    case reply_no_content: return "HTTP/1.1 204 No Content\r\n"; break;
    case reply_multiple_choices: return "HTTP/1.1 300 Multiple Choices\r\n"; break;
    case reply_moved_permanently: return "HTTP/1.1 301 Moved Permanently\r\n"; break;
    case reply_moved_temporarily: return "HTTP/1.1 302 Moved Temporarily\r\n"; break;
    case reply_not_modified: return "HTTP/1.1 304 Not Modified\r\n"; break;
    case reply_bad_request: return "HTTP/1.1 400 Bad Request\r\n"; break;
    case reply_unauthorized: return "HTTP/1.1 401 Unauthorized\r\n"; break;
    case reply_forbidden: return "HTTP/1.1 403 Forbidden\r\n"; break;
    case reply_not_found: return "HTTP/1.1 404 Not Found\r\n"; break;
    case reply_internal_server_error: return "HTTP/1.1 500 Internal Server Error\r\n"; break;
    case reply_not_implemented: return "HTTP/1.1 501 Not Implemented\r\n"; break;
    case reply_bad_gateway: return "HTTP/1.1 502 Bad Gateway\r\n"; break;
    case reply_service_unavailable: return "HTTP/1.1 503 Service Unavailable\r\n"; break;
    default: return "HTTP/1.1 200 OK\r\n"; break;
    }
}
