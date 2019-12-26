//
// Created by tom on 06.12.19.
//
#include <stdio.h>
#include <stdlib.h>

#include "future.h"

typedef struct tuple {
    long long int old_value;
    int multiplier;
} tuple_t;

void *multiply(void *what, size_t what_size, size_t *res_size_ptr) {
    tuple_t *what_tuple = (tuple_t *) what;
    what_tuple->multiplier++;
    what_tuple->old_value *= what_tuple->multiplier;

    if (res_size_ptr != NULL) {
        *res_size_ptr = what_size;
    }

    return what_tuple;
}

void print_answer(long long int res) {
    printf("%lld\n", res);
}

int main() {
    int n;
    // Incorrect input is intentionally left triggering UB since there's no specification in the task what to do
    scanf("%d", &n);  // NOLINT(cert-err34-c)

    // Special case for small `n`s where starting up the thread pool makes no sense
    if (n == 0 || n == 1) {
        print_answer(1);
        return 0;
    }

    thread_pool_t pool;
    thread_pool_init(&pool, 3);

    callable_t callable;
    callable.function = multiply;
    tuple_t acc;
    acc.multiplier = 1;
    acc.old_value = 1;
    callable.arg = &acc;
    callable.argsz = sizeof(tuple_t);

    future_t futures[n - 1];

    // TODO: comment
    // Error codes are not checked since there's nothing in the task spec. on what to do with them
    // If an error occurs (tho it shouldn't), it will trigger UB and we are hoping for the best
    async(&pool, &futures[0], callable);
    for (int i = 0; i < n - 2; i++) {
        map(&pool, &futures[i + 1], &futures[i], multiply);
    }
    await(&futures[n - 2]);

    print_answer(acc.old_value);

    thread_pool_destroy(&pool);

    return 0; // TODO: check if we don;t need to scanf again // TODO: memory leak of not-uninitialised futures
}