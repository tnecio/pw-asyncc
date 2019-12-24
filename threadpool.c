#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "threadpool.h"

// This is the function that describes the whole life of a worker thread
// On error: silently ignore and hope for the best
void *worker_life(void *parent_pool_) {
    thread_pool_t *parent_pool = (thread_pool_t *) parent_pool_;
    runnable_t *job;

    while (parent_pool->keep_working) {
        // Wait till there is something to do
        silent_on_err(pthread_mutex_lock(&parent_pool->jobs_to_do_mutex));

        while(parent_pool->jobs_to_do_counter == 0 && parent_pool->keep_working) {
            silent_on_err(pthread_cond_wait(&parent_pool->stg_to_do_cond, &parent_pool->jobs_to_do_mutex));
        }
        // If got out on jobs_to_do_counter > 0: book yourself a job
        // If got out on !keep_working: jobs don't matter anymore for anyone
        (parent_pool->jobs_to_do_counter)--;

        // Signal that work is finished to the pool-destroying thread if we are the last worker to run
        if (parent_pool->jobs_to_do_counter == 0 && !parent_pool->accepts_work) {
            silent_on_err(pthread_cond_signal(&parent_pool->work_finished));
        }
        silent_on_err(pthread_mutex_unlock(&parent_pool->jobs_to_do_mutex));


        if (!parent_pool->keep_working) {
            // pool_destroy could have been called when we were waiting
            // yes, there could be actual jobs in jobqueue, they will be lost purposefully on pool_destroy
            break;
        }

        silent_on_err(pthread_mutex_lock(&parent_pool->jobqueue_mutex));
        // We know for 100% there is a job for us because:
        // 1. If we are called from `pool_destroy` then keep_working MUST already be false, so we broke earlier
        // 2. So, we are called from `defer` and we know each defer puts one job into queue and increments jobs_to_do
        // that we just decremented (and we did this in a mutex so we could not decrement it together with some other
        // thread.
        job = queue_pop(parent_pool->jobqueue);
        silent_on_err(pthread_mutex_unlock(&parent_pool->jobqueue_mutex));

        (job->function)(job->arg, job->argsz);
        free(job); // Job must have been allocated in `defer`
    }

    return NULL;
}

int thread_pool_init(thread_pool_t *pool, size_t pool_size) {
    if (pool == NULL) {
        return NULL_POINTER_ERROR;
    } else if (pool_size == 0) {
        return ZERO_THREADS_ERROR;
    }

    pool->threads = (pthread_t *) calloc(pool_size, sizeof(pthread_t)); // TODO: zoptymalizować calloci
    if (pool->threads == NULL) {
        return MEMORY_ALLOCATION_ERROR;
    }

    pool->size = pool_size;
    pool->keep_working = true;
    pool->accepts_work = true;
    return_on_err(pthread_cond_init(&pool->work_finished, NULL));
    return_on_err(pthread_cond_init(&pool->stg_to_do_cond, NULL));
    return_on_err(pthread_mutex_init(&pool->jobs_to_do_mutex, NULL));
    pool->jobs_to_do_counter = 0;
    return_on_err(pthread_mutex_init(&pool->jobqueue_mutex, NULL));
    pool->jobqueue = queue_init();

    for (size_t i = 0; i < pool_size; i++) {
        return_on_err(pthread_create(&pool->threads[i], NULL, worker_life, pool));
    }

    return 0;
}

void thread_pool_destroy_fast(struct thread_pool *pool) {
    pool->accepts_work = false;
    // Signal all threads it's time for suicide
    pool->keep_working = false;

    // Call all workers to check that they have something to do
    silent_on_err(pthread_mutex_lock(&pool->jobs_to_do_mutex));
    silent_on_err(pthread_cond_broadcast(&pool->stg_to_do_cond));
    silent_on_err(pthread_mutex_unlock(&pool->jobs_to_do_mutex));

    // Wait till all workers die
    void *res = NULL;
    for (size_t i = 0; i < pool->size; i++) {
        silent_on_err(pthread_join(pool->threads[i], res));
    }

    // Free allocated memory
    runnable_t *runnable = NULL;
    while (!queue_empty(pool->jobqueue)) {
        runnable = queue_pop(pool->jobqueue);
        free(runnable);
    }
    queue_destroy(pool->jobqueue);
    silent_on_err(pthread_cond_destroy(&pool->stg_to_do_cond));
    silent_on_err(pthread_cond_destroy(&pool->work_finished));
    silent_on_err(pthread_mutex_destroy(&pool->jobqueue_mutex));
    silent_on_err(pthread_mutex_destroy(&pool->jobs_to_do_mutex));
    free(pool->threads);
}

void thread_pool_destroy(thread_pool_t *pool) {
    silent_on_err(pthread_mutex_lock(&pool->jobqueue_mutex));
    pool->accepts_work = false;
    silent_on_err(pthread_mutex_unlock(&pool->jobqueue_mutex));

    silent_on_err(pthread_mutex_lock(&pool->jobs_to_do_mutex));
    while(pool->jobs_to_do_counter > 0) {
        silent_on_err(pthread_cond_wait(&pool->work_finished, &pool->jobs_to_do_mutex));
    }
    silent_on_err(pthread_mutex_unlock(&pool->jobs_to_do_mutex));

    thread_pool_destroy_fast(pool);
}

int defer(struct thread_pool *pool, runnable_t runnable) {
    if (pool == NULL) {
        return NULL_POINTER_ERROR;
    }

    return_on_err(pthread_mutex_lock(&pool->jobqueue_mutex));

    if (!pool->accepts_work) {
        return_on_err(pthread_mutex_unlock(&pool->jobqueue_mutex));
        return CLOSED_POOL_ERROR;

    } else {
        // So that `runnable` doesn't fly away, we copy it to our own memory
        // Thread that will take up this job will be responsible for the disposal of this allocation
        runnable_t* runnable_ptr = (runnable_t*) calloc(1, sizeof(runnable_t));
        if (runnable_ptr==NULL) {
            return_on_err(pthread_mutex_unlock(&pool->jobqueue_mutex));
            return MEMORY_ALLOCATION_ERROR;
        }

        *runnable_ptr = runnable;

        int err = queue_push(pool->jobqueue, runnable_ptr);
        if (err != 0) {
            return err;
        }
        return_on_err(pthread_mutex_unlock(&pool->jobqueue_mutex));
    }

    return_on_err(pthread_mutex_lock(&pool->jobs_to_do_mutex));
    (pool->jobs_to_do_counter)++;
    return_on_err(pthread_mutex_unlock(&pool->jobs_to_do_mutex));

    return_on_err(pthread_cond_signal(&pool->stg_to_do_cond));

    return 0;
}
