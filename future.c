#include "err.h"
#include "future.h"

future_t future_make(callable_t callable) {
    // No allocations here, but there is pthread mutex and cond creation!
    future_t res;
    res.callable = callable;
    res.done = false;
    pthread_safe(pthread_cond_init(&(res.done_cond), NULL));
    pthread_safe(pthread_mutex_init(&(res.done_mutex), NULL));
    res.res = NULL;
    res.res_size = -1;
    return res;
}

void future_destroy(future_t *future) {
    pthread_safe(pthread_cond_destroy(&(future->done_cond)));
    pthread_safe(pthread_mutex_destroy(&(future->done_mutex)));
}

void async_work(void *arg, size_t argsz __attribute__((unused))) {
    // Extract function to run
    future_t *future = (future_t *) arg;
    callable_t callable = future->callable;
    callable_function_t function = callable.function;

    // Run the function and store the result
    void *res = (*function)(callable.arg, callable.argsz, &(future->res_size));
    future->res = res;

    // Signal end of work to all who are interested
    pthread_safe(pthread_mutex_lock(&(future->done_mutex)));
    future->done = true;
    pthread_safe(pthread_cond_broadcast(&(future->done_cond)));
    pthread_safe(pthread_mutex_unlock(&(future->done_mutex)));

}

int async(thread_pool_t *pool, future_t *future, callable_t callable) {
    // 1. Create a Future
    *future = future_make(callable);

    // 2. Send the task to the pool
    runnable_t runnable;
    runnable.function = async_work;
    runnable.arg = future; // responsibility for allocation is on the user
    runnable.argsz = sizeof(future);

    defer(pool, runnable);

    return 0; // TODO: in future, return error instead of failing
}

typedef struct futures_tuple {
    future_t *old;
    future_t *new;
} futures_tuple_t;

void map_work(void *arg, size_t argsz __attribute__((unused))) {
    futures_tuple_t *futures_tuple_ptr = (futures_tuple_t *) arg;
    future_t *old = futures_tuple_ptr->old;
    future_t *new = futures_tuple_ptr->new;

    // Wait for the result of the first calculation
    await(old);

    new->callable.arg = old->res;
    new->callable.argsz = old->res_size;

    async_work(new, sizeof(new));

    free(futures_tuple_ptr); // Allocation in `map`
}

int map(thread_pool_t *pool, future_t *future, future_t *from, callable_function_t function) {
    // 1. Make function that will defer to the pool upon future getting done
    // It needs to know: the new future with `function`; the old future;

    // Allocation so it doesn't fly away (remember: user is responsible for the `args` memory in `defer`)
    futures_tuple_t *futures_tuple_ptr = (futures_tuple_t *) calloc(1, sizeof(futures_tuple_t));
    futures_tuple_t futures_tuple;
    if (futures_tuple_ptr == NULL) {
        failure(0, "Memory allocation");
    } else {
        futures_tuple = *futures_tuple_ptr;
    }
    futures_tuple.old = from;

    callable_t callable;
    callable.function = function;
    callable.argsz = 0;
    callable.arg = NULL;
    *future = future_make(callable);
    futures_tuple.new = future;

    runnable_t runnable;
    runnable.arg = futures_tuple_ptr;
    runnable.argsz = sizeof(futures_tuple);
    runnable.function = map_work;

    return  defer(pool, runnable);
}

void *await(future_t *future) {
    pthread_safe(pthread_mutex_lock(&(future->done_mutex)));
    while (!future->done) {
        pthread_safe(pthread_cond_wait(&(future->done_cond), &(future->done_mutex)));
    }
    pthread_safe(pthread_mutex_unlock(&(future->done_mutex)));

    return future->res;
}
