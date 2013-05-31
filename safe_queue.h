#ifndef SAFE_LIST_H
#define SAFE_LIST_H
#include "malloc.h"

#define SAFE_QUEUE_OK 1
#define SAFE_QUEUE_ERR 0

typedef struct safeQueueEntry_t {
    void *value;
    struct safeQueueEntry_t *next;
} safeQueueEntry;

typedef struct {
    safeQueueEntry *head;
    safeQueueEntry *tail;
    int lastNodeRetrieved;
    void (*free)(void *ptr);
} safeQueue;


safeQueue *safeQueueCreate();
void safeQueueRelease(safeQueue **q_in);
int safeQueuePush(safeQueue *ll, void *value);
void * safeQueuePop(safeQueue *ll);

#endif // SAFE_LIST_H
