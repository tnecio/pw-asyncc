#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
//#include <errno.h> // TODO: niedozwolony plik nagłówkowy

#include "threadpool.h"


void *worker_life(void *parent_pool_as_void) {
    // This is the function that describes the whole life of a worker thread:
    // waiting for job, doing a job, killing itself, etc.

    thread_pool_t *parent_pool = (thread_pool_t *) parent_pool_as_void;

    runnable_t *job;

    while (parent_pool->keep_working) {
        // Wait till there is something to do
        pthread_mutex_lock(&(parent_pool->jobs_to_do_mutex));
        while(parent_pool->jobs_to_do_counter == 0 && parent_pool->keep_working) {
            pthread_cond_wait(&(parent_pool->stg_to_do_cond), &(parent_pool->jobs_to_do_mutex)); // TODO: handle return value
        }
        // If got out on jobs_to_do_counter > 0: book yourself a job
        // If got out on !keep_working: jobs don't matter anymore for anyone
        (parent_pool->jobs_to_do_counter)--;
        pthread_mutex_unlock(&(parent_pool->jobs_to_do_mutex));

        if (!parent_pool->keep_working) {
            // pool_destroy could have been called when we were waiting
            // yes, there could be actual jobs in jobqueue, they will be lost purposefully on pool_destroy
            break;
        }

        pthread_mutex_lock(&(parent_pool->jobqueue_mutex));
        // We know for 100% there is a job for us because:
        // 1. If we are called from `pool_destroy` then keep_working MUST already be false, so we broke earlier
        // 2. So, we are called from `defer` and we know each defer puts one job into queue and increments jobs_to_do
        // that we just decremented (and we did this in a mutex so we could not decrement it together with some other
        // thread.
        job = queue_pop(parent_pool->jobqueue);
        pthread_mutex_unlock(&(parent_pool->jobqueue_mutex));

        (job->function)(job->arg, job->argsz);
        free(job);
    }

    return NULL;
}

int thread_pool_init(thread_pool_t *pool, size_t pool_size) {
    pool->threads = (pthread_t *) calloc(pool_size, sizeof(pthread_t)); // TODO: zoptymalizować calloci
    pool->size = pool_size;
    pool->keep_working = true;
    pthread_cond_init(&(pool->stg_to_do_cond), NULL);
    pthread_mutex_init(&(pool->jobs_to_do_mutex), NULL);
    pool->jobs_to_do_counter = 0;
    pthread_mutex_init(&(pool->jobqueue_mutex), NULL);
    pool->jobqueue = queue_init();

    for (size_t i = 0; i < pool_size; i++) {
        pthread_create(&(pool->threads[i]), NULL, worker_life, pool); // TODO: check return value
    }

    return 0; // TODO: robić coś z return value?
}

void thread_pool_destroy(struct thread_pool *pool) {
    // Signal all threads it's time for suicide
    pool->keep_working = false;

    // Call all workers to check that they have something to do
    pthread_mutex_lock(&(pool->jobs_to_do_mutex));
    pthread_cond_broadcast(&(pool->stg_to_do_cond));
    pthread_mutex_unlock(&(pool->jobs_to_do_mutex));

    // Wait till all workers die
    void *res = NULL;
    for (size_t i = 0; i < pool->size; i++) {
        pthread_join(pool->threads[i], res);
    }

    // Free allocated memory
    pthread_cond_destroy(&(pool->stg_to_do_cond));
    runnable_t *runnable = NULL;
    while (!queue_empty(pool->jobqueue)) {
        runnable = queue_pop(pool->jobqueue);
        free(runnable);
    }
    queue_destroy(pool->jobqueue);
    free(pool->threads);
}

int defer(struct thread_pool *pool, runnable_t runnable) {
    pthread_mutex_lock(&(pool->jobqueue_mutex));
    // So that `runnable` doesn't fly away: // TODO: any better ideas?
    runnable_t *runnable_ptr = (runnable_t *) calloc(1, sizeof(runnable_t));
    *runnable_ptr = runnable;
    queue_push(pool->jobqueue, runnable_ptr);
    pthread_mutex_unlock(&(pool->jobqueue_mutex));

    pthread_mutex_lock(&(pool->jobs_to_do_mutex));
    (pool->jobs_to_do_counter)++;
    pthread_mutex_unlock(&(pool->jobs_to_do_mutex));

    pthread_cond_signal(&(pool->stg_to_do_cond));

    return 0;
}
