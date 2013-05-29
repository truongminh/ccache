#include "safe_list.h"

safeList * safeListCreate() {
    safeList *sl = malloc(sizeof(*sl));
    sl->head = sl->tail = malloc(sizeof(safeListNode));
    sl->tail->next = NULL;
    sl->free = NULL;
    return sl;
}

void safeListRelease(safeList **pp_ll) {
    safeListNode *curr_node;
    safeListNode *next_node;
    void *val_to_free;
    if(pp_ll&&*pp_ll) {
        safeList *ll = (safeList *)(*pp_ll);
        if(ll->free) {
            while((val_to_free = safeListGetHead(ll)))
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


int safeListAddNodeTail(safeList *ll, void *value) {
    safeListNode *node = malloc(sizeof(*node));
    if(node == NULL) return SAFE_LIST_ERR;
    node->value = value;
    node->next = NULL;
    ll->tail->next = node;
    ll->tail = node;
    if(ll->head == NULL) ll->head = ll->tail;
    return SAFE_LIST_OK;
}


void * safeListGetHead(safeList *ll) {
    if(ll->head == NULL) return NULL;
    void* re = ll->head->value;
    safeListNode *ln = ll->head;
    if(ll->head->next) free(ln);
    ll->head = ll->head->next;
    return re;
}

