#ifndef SAFE_LIST_H
#define SAFE_LIST_H
#include "malloc.h"

#define SAFE_LIST_OK 1
#define SAFE_LIST_ERR 0

typedef struct safeListNode_t {
    void *value;
    struct safeListNode_t *next;
} safeListNode;

typedef struct {
    safeListNode *head;
    safeListNode *tail;
    int lastNodeRetrieved;
    void (*free)(void *ptr);
} safeList;


safeList *safeListCreate();
void safeListRelease(safeList **q_in);
int safeListAddNodeTail(safeList *ll, void *value);
void * safeListGetHead(safeList *ll);

#endif // SAFE_LIST_H
