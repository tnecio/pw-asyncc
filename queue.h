//
// Created by tom on 04.12.19.
//

#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <stddef.h>
#include <stdbool.h>

typedef struct queue_node {
    void *element; // Pointer to data stored at this node
    struct queue_node *next;
    struct queue_node *prev;
} queue_node_t;

typedef struct queue {
    size_t size; // Number of elements in the queue
    queue_node_t *first;
    queue_node_t *last;
} queue_t;

queue_t *queue_init();

void queue_destroy(queue_t *queue);

void queue_push(queue_t *queue, void *element);

void *queue_pop(queue_t *queue); // Returns NULL on empty queue

bool queue_empty(queue_t *queue);

#endif //_QUEUE_H_
