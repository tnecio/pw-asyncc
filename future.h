#ifndef FUTURE_H
#define FUTURE_H

#include "threadpool.h"

typedef void *(*callable_function_t)(void *, size_t, size_t *);

typedef struct callable {
    callable_function_t function;
    void *arg;
    size_t argsz;
} callable_t;

typedef struct future {
    callable_t callable;
    void *res;
    size_t res_size;

    // `done` and `done_cond` can be accessed concurrently, so we'll protect them with a mutex
    pthread_mutex_t done_mutex;
    pthread_cond_t done_cond;
    bool done;
} future_t;

int async(thread_pool_t *pool, future_t *future, callable_t callable);

int map(thread_pool_t *pool, future_t *future, future_t *from, callable_function_t function);

void *await(future_t *future);

// User might have a reference to the future and use it in `map` many times in many threads etc.
// therefore we don't know when to clean up a future – it must be the user's responsibility
void future_destroy(future_t *future);

#endif
