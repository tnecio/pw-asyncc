//
// Created by tom on 04.12.19.
//

#define tn_test(condition, title) \
    if (!(condition)) { \
        printf(title); \
        printf(" FAILED!\n"); \
        failed++; \
    } else { \
        printf(title); \
        printf(" PASSED!\n"); \
        passed++; \
    }

#include <stdio.h>

#include "queue.h"

int main() {
    int failed = 0;
    int passed = 0;

    queue_t *q = queue_init();
    char letters[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};
    queue_push(q, &(letters[1])); // B
    queue_push(q, &(letters[0])); // A

    tn_test(!queue_empty(q), "Test Queue Empty 1")

    char *letter_B = queue_pop(q);
    tn_test(*letter_B == 'B', "Test Queue Pop B")

    tn_test(!queue_empty(q), "Test Queue Empty 2")

    queue_push(q, &(letters[2])); // C
    char *letter_A = queue_pop(q);
    tn_test(*letter_A == 'A', "Test Queue Pop A")

    tn_test(!queue_empty(q), "Test Queue Empty 3")

    char *letter_C = queue_pop(q);
    tn_test(*letter_C == 'C', "Test Queue Pop C")

    tn_test(queue_empty(q), "Test Queue Empty 4")

    char *null_pointer = queue_pop(q);
    tn_test(null_pointer == NULL, "Test Queue Pop NULL")

    tn_test(queue_empty(q), "Test Queue Empty 5")

    queue_destroy(q); // Destroying empty queue

    q = queue_init();
    queue_push(q, &(letters[1])); // B
    queue_push(q, &(letters[0])); // A

    queue_destroy(q); // Destroying non-empty queue

    printf("\n\nTests failed: %d\nTests passed: %d\n\n", failed, passed);
}