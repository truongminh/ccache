/* safe_queue.c - lock-free queue allowing one producer and one consumer
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

