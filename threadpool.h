#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <stddef.h>
#include <stdbool.h> // TODO: niedozwolony plik nagłówkowy
#include <pthread.h>

#include "err.h"
#include "queue.h"

typedef struct runnable {
    void (*function)(void *, size_t); // Pointer to function: void function(void *arg, size_t argsz)
    void *arg;
    size_t argsz;
} runnable_t;

typedef struct thread_pool {
    pthread_t *threads;
    size_t size;
    volatile bool keep_working;
    volatile bool accepts_work;
    pthread_cond_t work_finished; // TODO check those volatiles
    pthread_cond_t stg_to_do_cond;
    pthread_mutex_t jobs_to_do_mutex;
    size_t jobs_to_do_counter;
    pthread_mutex_t jobqueue_mutex;
    queue_t *jobqueue;
} thread_pool_t;

// Initialises the threadpool of `pool_size` threads in memory pointed to by `pool`
// Returns error code, or 0 on success
int thread_pool_init(thread_pool_t *pool, size_t pool_size);

// Waits for all jobs to finish and then destroys the pool (or does its best to do so)
// Ignores silently all pthread errors
// `pool` must not be NULL
void thread_pool_destroy(thread_pool_t *pool);

// Destroys thread pool discarding those jobs that have not yet started running
// Ignores silently all pthread errors
// `pool` must not be NULL
void thread_pool_destroy_fast(thread_pool_t *pool);

// Defers a job described by `runnable` to thread pool in `pool`
// Returns error code, or 0 on success
// If running a job encounters an error in pthreads, it silently ignores it
int defer(thread_pool_t *pool, runnable_t runnable);

#endif
