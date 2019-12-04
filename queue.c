//
// Created by tom on 04.12.19.
//

#include <stdlib.h>

#include "queue.h"


queue_t *queue_init() {
    queue_t *res = (queue_t *) calloc(1, sizeof(queue_t));
    res->first = NULL;
    res->last = NULL;
    res->size = 0;
    return res;
}

void queue_destroy(queue_t *queue) {
    while (queue->size > 0) {
        queue_pop(queue);
    }
    free(queue);
}

void queue_push(queue_t *queue, void *element) {
    queue_node_t *node = (queue_node_t *) calloc(1, sizeof(queue_node_t));
    node->element = element;
    node->next = NULL;
    node->prev = queue->last;

    if (queue->size == 0) {
        queue->first = node;
    } else {
        queue->last->next = node;
    }

    queue->last = node;
    queue->size++;
}

void *queue_pop(queue_t *queue) {
    if (queue->size == 0) {
        return NULL;
    }
    queue_node_t *node = queue->first;
    void *res = node->element;

    if (queue->size == 1) {
        queue->last = NULL;
    } else {
        node->next->prev = NULL;
    }

    queue->first = node->next;
    queue->size--;
    free(node);
    return res;
}

bool queue_empty(queue_t *queue) {
    return queue->size == 0;
}