#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <stddef.h>
#include <stdbool.h>

typedef struct queue_node {
    void *element; // Pointer to data stored at this node
    struct queue_node *next;
} queue_node_t;

typedef struct queue {
    size_t size; // Number of elements in the queue
    queue_node_t *first;
    queue_node_t *last;
} queue_t;

// Initialise queue and return pointer to it (NULL on error)
queue_t *queue_init();

// Destroy the queue.
void queue_destroy(queue_t *queue);

// Push element pointer to queue.
// Return error code, 0 on success
int queue_push(queue_t *queue, void *element);

// Pops element pointer from the queue (NULL if queue is empty)
void *queue_pop(queue_t *queue);

// Returns whether queue is empty
bool queue_empty(queue_t *queue);

#endif //_QUEUE_H_
