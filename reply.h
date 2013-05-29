#ifndef REPLY_H
#define REPLY_H
#include "adlist.h"
#include "sds.h"
#include "dict.h"

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
