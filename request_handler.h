#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include "request.h"
#include "reply.h"
#include "cache.h"

#define HANDLER_OK 0
#define HANDLER_ERR 1
#define HANDLER_BLOCK 2


void requestHandleInitializeGlobalCache();

int requestHandle(request *req, reply *rep, ccache *c);

void requestHandleError(request *req, reply *rep);

#endif // REQUEST_HANDLER_H
