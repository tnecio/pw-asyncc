#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "threadpool.h"

// This is the function that describes the worker thread
// On error: silently ignore and hope for the best
// Always returns NULL
void *worker(void *parent_pool_) {
    // Get pointer to the thread pool
    thread_pool_t *parent_pool = (thread_pool_t *) parent_pool_;

    runnable_t *job;
    for (;;) {
        // Wait until there is something to do
        silent_on_err(pthread_mutex_lock(&parent_pool->jobs_mutex));
        while (queue_empty(parent_pool->jobqueue) && parent_pool->keep_working) {
            silent_on_err(pthread_cond_wait(&parent_pool->stg_to_do_cond, &parent_pool->jobs_mutex));
        }

        // "Book" a job (or get NULL if we "got out" on `keep_working` being false)
        job = queue_pop(parent_pool->jobqueue);
        silent_on_err(pthread_mutex_unlock(&parent_pool->jobs_mutex));

        if (job == NULL) { // no job -> !`keep_working` -> time to finish work
            return NULL;
        } // else: job was initialised, time to do it
        job->function(job->arg, job->argsz);
        free(job); // Job must have been allocated in `defer`
    }
}

// Initialises the threadpool of `pool_size` threads in memory pointed to by `pool`
// Returns error code, or 0 on success
int thread_pool_init(thread_pool_t *pool, size_t pool_size) {
    if (pool == NULL) {
        return NULL_POINTER_ERROR;
    } else if (pool_size == 0) {
        return ZERO_THREADS_ERROR;
    }

    pool->threads = (pthread_t *) calloc(pool_size, sizeof(pthread_t));
    if (pool->threads == NULL) {
        return MEMORY_ALLOCATION_ERROR;
    }
    pool->thread_count = pool_size;

    return_on_err(pthread_mutex_init(&pool->jobs_mutex, NULL));
    return_on_err(pthread_cond_init(&pool->stg_to_do_cond, NULL));

    pool->keep_working = true;
    pool->jobqueue = queue_init();

    for (size_t i = 0; i < pool_size; i++) {
        return_on_err(pthread_create(&pool->threads[i], NULL, worker, pool));
    }

    return 0;
}

// Waits for all jobs to finish and then destroys the pool
// Ignores silently all pthread errors
// `pool` must not be NULL
void thread_pool_destroy(thread_pool_t *pool) {
    silent_on_err(pthread_mutex_lock(&pool->jobs_mutex));
    pool->keep_working = false;

    // Signal waiting threads to check that they can stop
    silent_on_err(pthread_cond_broadcast(&pool->stg_to_do_cond));
    silent_on_err(pthread_mutex_unlock(&pool->jobs_mutex));

    // Wait until all workers stop
    for (size_t i = 0; i < pool->thread_count; i++) {
        silent_on_err(pthread_join(pool->threads[i], NULL));
    }

    // Free allocated memory
    silent_on_err(pthread_mutex_destroy(&pool->jobs_mutex));
    silent_on_err(pthread_cond_destroy(&pool->stg_to_do_cond));
    queue_destroy(pool->jobqueue);
    free(pool->threads);
}

// Defers a job described by `runnable` to thread pool in `pool`
// Returns error code, or 0 on success
// If running a job encounters an error in pthreads, it silently ignores it
int defer(thread_pool_t *pool, runnable_t runnable) {
    if (pool == NULL) {
        return NULL_POINTER_ERROR;
    }

    return_on_err(pthread_mutex_lock(&pool->jobs_mutex));
    // `runnable` must be moved from this scope's stack to some memory that will be still accessible
    // when some thread eventually gets to work on it
    runnable_t *runnable_ptr = (runnable_t *) calloc(1, sizeof(runnable_t));
    if (runnable_ptr == NULL) {
        return_on_err(pthread_mutex_unlock(&pool->jobs_mutex));
        return MEMORY_ALLOCATION_ERROR;
    }
    *runnable_ptr = runnable;

    // Put runnable into jobqueue
    return_on_err(queue_push(pool->jobqueue, runnable_ptr));

    // Signal threads waiting for work
    return_on_err(pthread_cond_signal(&pool->stg_to_do_cond));

    return_on_err(pthread_mutex_unlock(&pool->jobs_mutex));
    return 0;
}