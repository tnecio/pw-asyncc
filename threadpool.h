#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <stddef.h>
#include <stdbool.h> // TODO: niedozwolony plik nagłówkowy
#include <pthread.h>

#include "queue.h"

typedef struct runnable {
    void (*function)(void *, size_t);
    void *arg;
    size_t argsz;
} runnable_t;

typedef struct thread_pool {
    pthread_t *threads;
    size_t size;
    volatile bool keep_working;
    pthread_cond_t stg_to_do_cond;
    pthread_mutex_t jobs_to_do_mutex;
    size_t jobs_to_do_counter;
    pthread_mutex_t jobqueue_mutex;
    queue_t *jobqueue;
} thread_pool_t;


int thread_pool_init(thread_pool_t *pool, size_t pool_size);

void thread_pool_destroy(thread_pool_t *pool);

int defer(thread_pool_t *pool, runnable_t runnable);

#endif
