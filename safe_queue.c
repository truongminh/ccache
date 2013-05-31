#include "safe_queue.h"

safeQueue * safeQueueCreate() {
    safeQueue *sl = malloc(sizeof(*sl));
    sl->head = NULL;
    sl->tail = malloc(sizeof(safeQueueEntry));
    sl->tail->next = NULL;
    sl->free = NULL;
    return sl;
}

void safeQueueRelease(safeQueue **pp_ll) {
    safeQueueEntry *curr_node;
    safeQueueEntry *next_node;
    void *val_to_free;
    if(pp_ll&&*pp_ll) {
        safeQueue *ll = (safeQueue *)(*pp_ll);
        if(ll->free) {
            while((val_to_free = safeQueuePop(ll)))
                ll->free(val_to_free);
        }
        curr_node = ll->head;
        while(curr_node) {
            next_node = curr_node->next;
            free(curr_node);
            curr_node = next_node;
        }
        free(ll);
        *pp_ll = NULL;
    }
}


int safeQueuePush(safeQueue *ll, void *value) {
    safeQueueEntry *node = malloc(sizeof(*node));
    if(node == NULL) return SAFE_QUEUE_ERR;
    node->value = value;
    node->next = NULL;
    ll->tail->next = node;
    ll->tail = node;
    if(ll->head == NULL) ll->head = ll->tail;
    return SAFE_QUEUE_OK;
}


void * safeQueuePop(safeQueue *ll) {
    if(ll->head == NULL) return NULL;    
    void* re = ll->head->value;
    safeQueueEntry *ln = ll->head;
    if(ll->head->next) free(ln);
    ll->head = ll->head->next;
    return re;
}

