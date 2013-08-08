/* reply.h - http reply
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

#ifndef REPLY_H
#define REPLY_H
#include "lib/adlist.h"
#include "lib/sds.h"
#include "lib/dict.h"

#define REPLY_OK DICT_OK
#define REPLY_ERR DICT_ERR

/* Unused arguments generate annoying warnings... */
#define REPLY_NOTUSED(V) ((void) V)

/// The status of the reply.
typedef enum
{
    reply_ok = 200,
    reply_created = 201,
    reply_accepted = 202,
    reply_no_content = 204,
    reply_multiple_choices = 300,
    reply_moved_permanently = 301,
    reply_moved_temporarily = 302,
    reply_not_modified = 304,
    reply_bad_request = 400,
    reply_unauthorized = 401,
    reply_forbidden = 403,
    reply_not_found = 404,
    reply_internal_server_error = 500,
    reply_not_implemented = 501,
    reply_bad_gateway = 502,
    reply_service_unavailable = 503
} reply_status_type;


/// A reply to be sent to a client.
typedef struct reply_t
{
    reply_status_type status;
    /// The headers to be included in the reply.
    dict *headers;
    /// The content to be sent in the reply.
    sds content;
    /// The output buffer could be used in cause we want to cache the reply
    sds obuf;
    int isCached;
} reply;

#define replyToBeCached(r) do{r->isCached = 1;}while(0);

reply* replyCreate();
void replyFree(reply *r);

sds replyToBuffer(reply* r);

int replyAddHeader(reply* r, const char* name, const char* value);

void replySetContent(reply*r , char *content);

void replySetStatus(reply*r , reply_status_type vStatus);

reply *replyStock(reply_status_type status);

void replyReset(reply *r);

char* replyStatusToString(reply_status_type status);

#endif // REPLY_H
