//
// Created by tom on 06.12.19.
//
#include <stdio.h>

#include "future.h"

typedef struct tuple {
    long long int value;
    int multiplier;
} tuple_t;

// This function takes tuple consisting of the last partial product and last multiplier in the 1 x 2 x 3 ... x n
// series, and updates this tuple
void *multiply(void *tuple_, size_t tuple_size, size_t *res_size_ptr) {
    tuple_t *tuple = (tuple_t *) tuple_;
    tuple->multiplier++;
    tuple->value *= tuple->multiplier;

    if (res_size_ptr != NULL) {
        *res_size_ptr = tuple_size;
    }

    return tuple;
}

// Helper one-liner to print result
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

    // Initialise thread pool
    thread_pool_t pool;
    thread_pool_init(&pool, 3);

    // Describe function to run (see `multiply` for details)
    callable_t callable;
    callable.function = multiply;

    // Initial values for factorial function
    tuple_t acc;
    acc.multiplier = 1; // We start with 1! already computed => n - 2 in the code below (Main part)
    acc.value = 1;

    callable.arg = &acc;
    callable.argsz = sizeof(tuple_t);

    // Declare space for all the Futures that will be used
    future_t futures[n - 1];

    // Error codes are not checked since there's nothing in the task spec. on what to do with them
    // If an error occurs (tho it shouldn't), it will trigger UB and we are hoping for the best

    // Main part that defers calculations to the threadpool
    async(&pool, &futures[0], callable);
    for (int i = 0; i < n - 2; i++) {
        map(&pool, &futures[i + 1], &futures[i], multiply);
    }

    // Wait for the final result
    await(&futures[n - 2]);
    print_answer(acc.value);

    // Clean up
    thread_pool_destroy(&pool);
    for (int i = 0; i < n - 1; i++) {
        future_destroy(&futures[i]);
    }

    return 0;
}