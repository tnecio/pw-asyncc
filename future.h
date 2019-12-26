#ifndef FUTURE_H
#define FUTURE_H

#include "threadpool.h"

// Alias to make code more readable
typedef void *(*callable_function_t)(void *, size_t, size_t *);

// Describes async task
typedef struct callable {
    callable_function_t function;
    void *arg;
    size_t argsz;
} callable_t;

// Represents result of calculation that might have not yet been completed; use `await` or `map` to use the result
typedef struct future {
    callable_t callable;
    void *res;
    size_t res_size;

    // `done` and `done_cond` can be accessed concurrently, so we'll protect them with a mutex
    pthread_mutex_t done_mutex;
    pthread_cond_t done_cond;
    bool done;
} future_t;


// Runs task described by `callable` asynchronously
// `pool` –> Pool that will execute the task
// `future` -> Memory place where a Future representing result will be written to
// `callable` -> Description of the task
// Return error code, or 0 on success
int async(thread_pool_t *pool, future_t *future, callable_t callable);

// Defer to `pool` a job that will call function `function` on the result of calculation
// described by `from`; Future describing result of the final calculation will be stored
// in space pointed to by `future`
// Return error code, or 0 on success
int map(thread_pool_t *pool, future_t *future, future_t *from, callable_function_t function);

// Wait for Future `future` to have its calculation complete;
// Returns pointer to the result
// Ignores silently errors
void *await(future_t *future);

// Destroys `future`
// Silently ignores all errors
void future_destroy(future_t *future);

#endif
