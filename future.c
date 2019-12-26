#include <stdlib.h>

#include "future.h"

// Creates a future_t in space pointed to by `future`, with result to be calculated by `callable`
// Returns error code, or 0 on success
int future_init(callable_t callable, future_t *future) {
    // No allocations here, but there is pthread mutex and cond creation!
    future_t res;
    res.callable = callable;
    res.res = NULL;
    res.res_size = 0;

    return_on_err(pthread_mutex_init(&(res.done_mutex), NULL));
    return_on_err(pthread_cond_init(&(res.done_cond), NULL));
    res.done = false;

    *future = res;

    return 0;
}

// Destroys `future`
// Silently ignores all errors
void future_destroy(future_t *future) {
    silent_on_err(pthread_mutex_destroy(&(future->done_mutex)));
    silent_on_err(pthread_cond_destroy(&(future->done_cond)));
}

// Job that actually calculates the result of `callable` in `future`, to be deferred to a thread pool
// Silently ignores all errors
void async_work(
        void *arg, // typeof(arg) == future_t
        size_t argsz __attribute__((unused)))
{
    // Extract function calculating the result
    future_t *future = (future_t *) arg;
    callable_t callable = future->callable;

    // Run the function
    future->res = (*callable.function)(callable.arg, callable.argsz, &future->res_size);

    // Signal end of work to all interested parties
    silent_on_err(pthread_mutex_lock(&future->done_mutex));
    future->done = true;
    silent_on_err(pthread_cond_broadcast(&future->done_cond));
    silent_on_err(pthread_mutex_unlock(&future->done_mutex));
}

// Runs task described by `callable` asynchronously
// `pool` –> Pool that will execute the task
// `future` -> Memory place where a Future representing result will be written to
// `callable` -> Description of the task
// Return error code, or 0 on success
int async(thread_pool_t *pool, future_t *future, callable_t callable) {
    if (pool == NULL || future == NULL) {
        return NULL_POINTER_ERROR;
    }

    // 1. Create a Future
    return_on_err(future_init(callable, future));

    // 2. Send the task to the pool
    runnable_t runnable;
    runnable.function = async_work;
    runnable.arg = future;
    runnable.argsz = sizeof(future_t);

    return defer(pool, runnable);
}

// Job that waits for one future to finish and then starts calculations on the second one using obtained result
// Silently ignores errors
void map_work(
        void *arg, // typeof(arg) == future_t[2]
        size_t argsz __attribute__((unused)))
{
    // Extract Futures
    future_t **futures = (future_t **) arg;
    future_t *old = futures[0];
    future_t *new = futures[1];

    // Wait for first calculation
    // TODO: it can happen that all but one threads in the threadpool await on some Future
    // TODO: while there is another that might have possibly started working but is waiting
    // TODO: in the jobqueue for the awaiting threads
    // TODO: Solution: instead of `await`ing, suspend execution and move to the back of the queue?
    await(old);

    // Run second calculation on the result of the first calculation
    new->callable.arg = old->res;
    new->callable.argsz = old->res_size;
    // re-use this thread to not get put at the end of the jobqueue
    async_work(new, sizeof(future_t));

    free(arg); // allocation in `map`
}

// Defer to `pool` a job that will call function `function` on the result of calculation
// described by `from`; Future describing result of the final calculation will be stored
// in space pointed to by `future`
// Return error code, or 0 on success
int map(thread_pool_t *pool, future_t *future, future_t *from, callable_function_t function) {
    if (pool == NULL || future == NULL || from == NULL) {
        return NULL_POINTER_ERROR;
    }

    // allocation of space for two Future pointers so that worker will read and write data to space pointed to by them
    // freeing will be done by `map_work`
    future_t **future_ptrs = (future_t **) calloc(2, sizeof(future_t *));
    if (future_ptrs == NULL) {
        return MEMORY_ALLOCATION_ERROR;
    }

    // copy pointers to Futures
    future_ptrs[0] = from;
    future_ptrs[1] = future;

    // prepare new future
    callable_t callable;
    callable.function = function;
    // callable.arg, .argsz will be set in `map_work` before running `async_work` with the callable
    return_on_err(future_init(callable, future_ptrs[1]));

    // prepare runnable with `map_work` as a function to run, and the two Futures as arg
    runnable_t runnable;
    runnable.arg = future_ptrs;
    runnable.argsz = 2 * sizeof(future_t);
    runnable.function = map_work;

    // Defering is last instruction; return its error code
    return defer(pool, runnable);
}

// Wait for Future `future` to have its calculation complete;
// Returns pointer to the result
// Ignores silently errors
void *await(future_t *future) {
    silent_on_err(pthread_mutex_lock(&future->done_mutex));
    while (!future->done) {
        silent_on_err(pthread_cond_wait(&future->done_cond, &future->done_mutex));
    }
    silent_on_err(pthread_mutex_unlock(&future->done_mutex));

    return future->res;
}
