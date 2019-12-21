#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err34-c"
//
// Created by tom on 06.12.19.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "threadpool.h"
#include "err.h"

typedef struct matrix_cell {
    int value;
    int time; // in milliseconds
} matrix_cell_t;

typedef struct job_description {
    matrix_cell_t matrix_cell;
    int res;
} job_description_t;

void cell_worker(void *arg, size_t argsz __attribute__((unused))) {
    job_description_t *job = (job_description_t *) arg;
    matrix_cell_t cell = job->matrix_cell;
    usleep(cell.time * 1000);
    job->res = cell.value;
}

int main() {
    int k, n;
    scanf("%d", &k);
    scanf("%d", &n);

    job_description_t *jobs_matrix = (job_description_t *) calloc(k * n, sizeof(job_description_t));

    thread_pool_t pool;
    if (thread_pool_init(&pool, 4)) {
        failure(0, "Thread pool initialisation failed");
    }

    int v, t;
    matrix_cell_t cell;
    job_description_t job_description;
    runnable_t job;
    for (int i = 0; i < k * n; i++) {
        scanf("%d %d", &v, &t);
        cell.value = v;
        cell.time = t;
        job_description.matrix_cell = cell;
        jobs_matrix[i] = job_description;

        job.function = cell_worker;
        job.arg = &jobs_matrix[i];
        job.argsz = sizeof(job_description);

        defer(&pool, job);
    }

    thread_pool_destroy(&pool);

    int sum;
    for (int i = 0; i < k; i++) {
        sum = 0;
        for (int j = 0; j < n; j++) {
            sum += jobs_matrix[i * n + j].res;
        }
        printf("%d\n", sum);
    }

    free(jobs_matrix);

    return 0;
}
#pragma clang diagnostic pop