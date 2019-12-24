#include <stdlib.h>
#include "err.h"
#include "future.h"

// Creates a future_t in space pointed to by `to`, with result to be calculated by `callable`
// Returns error code, or 0 on success
int future_init(callable_t callable, future_t *to) {
    // No allocations here, but there is pthread mutex and cond creation!
    future_t res;
    res.callable = callable;
    res.done = false;
    return_on_err(pthread_cond_init(&(res.done_cond), NULL));
    return_on_err(pthread_mutex_init(&(res.done_mutex), NULL));
    res.res = NULL;
    res.res_size = -1;
    *to = res;

    return 0;
}

// Destroys future
// Silently ignores all errors
void future_destroy(future_t *future) {
    silent_on_err(pthread_cond_destroy(&(future->done_cond)));
    silent_on_err(pthread_mutex_destroy(&(future->done_mutex)));
}

// Job that actually calculates the result of `callable` in `future`, to be deferred to a thread pool
// Silently ignores all errors
void async_work(void *arg, size_t argsz __attribute__((unused))) {
    // Extract function to run
    future_t *future = (future_t *) arg;
    callable_t callable = future->callable;
    callable_function_t function = callable.function;

    // Run the function and store the result
    void *res = (*function)(callable.arg, callable.argsz, &(future->res_size));
    future->res = res;

    // Signal end of work to all who are interested
    silent_on_err(pthread_mutex_lock(&(future->done_mutex)));
    future->done = true;
    silent_on_err(pthread_cond_broadcast(&(future->done_cond)));
    silent_on_err(pthread_mutex_unlock(&(future->done_mutex)));

}

int async(thread_pool_t *pool, future_t *future, callable_t callable) {
    // 1. Create a Future
    return_on_err(future_init(callable, future));

    // 2. Send the task to the pool
    runnable_t runnable;
    runnable.function = async_work;
    runnable.arg = future; // responsibility for allocation is on the user
    runnable.argsz = sizeof(future_t);

    return defer(pool, runnable);
}

typedef struct futures_tuple {
    future_t *old;
    future_t *new;
} futures_tuple_t;

// Job that waits for one future to finish and then starts calculations on the second one using obtained result
// Silently ignores errors
void map_work(void *arg, size_t argsz __attribute__((unused))) {
    futures_tuple_t *futures_tuple_ptr = (futures_tuple_t *) arg;
    future_t *old = futures_tuple_ptr->old;
    future_t *new = futures_tuple_ptr->new;

    // Wait for the result of the first calculation
    await(old); // TODO: ensure that await is thread-safe

    new->callable.arg = old->res;
    new->callable.argsz = old->res_size;

    async_work(new, sizeof(future_t));

    free(futures_tuple_ptr); // Allocation in `map`
}

int map(thread_pool_t *pool, future_t *future, future_t *from, callable_function_t function) {
    // 1. Make function that will defer to the pool upon future getting done
    // It needs to know: the new future with `function`; the old future;
    if (pool == NULL || future == NULL || from == NULL) {
        return NULL_POINTER_ERROR;
    }

    // Allocation so it doesn't fly away (user is responsible for the `args` memory in `defer`)
    futures_tuple_t *futures_tuple = (futures_tuple_t *) calloc(1, sizeof(futures_tuple_t));
    if (futures_tuple == NULL) {
        return MEMORY_ALLOCATION_ERROR;
    }
    futures_tuple->old = from;

    callable_t callable;
    callable.function = function;
    callable.argsz = 0;
    callable.arg = NULL;
    return_on_err(future_init(callable, future));
    futures_tuple->new = future;

    runnable_t runnable;
    runnable.arg = futures_tuple;
    runnable.argsz = sizeof(futures_tuple_t);
    runnable.function = map_work;

    return defer(pool, runnable);
}

void *await(future_t *future) {
    silent_on_err(pthread_mutex_lock(&(future->done_mutex)));
    while (!future->done) {
        silent_on_err(pthread_cond_wait(&(future->done_cond), &(future->done_mutex)));
    }
    silent_on_err(pthread_mutex_unlock(&(future->done_mutex)));

    return future->res;
}
