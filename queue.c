#include <stdlib.h>

#include "queue.h"
#include "err.h"

// Initialise queue and return pointer to it (NULL on error)
queue_t *queue_init() {
    queue_t *res = (queue_t *) calloc(1, sizeof(queue_t));
    if (res == NULL) {
        return NULL;
    }

    res->first = NULL;
    res->last = NULL;
    res->size = 0;
    return res;
}

// Destroy the queue.
void queue_destroy(queue_t *queue) {
    while (queue->size > 0) {
        queue_pop(queue);
    }
    free(queue);
}

// Push element pointer to queue.
// Return error code, 0 on success
int queue_push(queue_t *queue, void *element) {
    queue_node_t *node = (queue_node_t *) calloc(1, sizeof(queue_node_t));
    if (node == NULL) {
        return MEMORY_ALLOCATION_ERROR;
    }

    node->element = element;
    node->next = NULL;

    if (queue->size == 0) {
        queue->first = node;
    } else {
        queue->last->next = node;
    }

    queue->last = node;
    queue->size++;

    return 0;
}

// Pops element pointer from the queue (NULL if queue is empty)
void *queue_pop(queue_t *queue) {
    if (queue->size == 0) {
        return NULL;
    }
    queue_node_t *node = queue->first;
    void *res = node->element;

    if (queue->size == 1) {
        queue->last = NULL;
    }

    queue->first = node->next;
    queue->size--;
    free(node); // allocation in `queue_push`
    return res;
}

// Returns whether if queue is empty
bool queue_empty(queue_t *queue) {
    return queue->size == 0;
}