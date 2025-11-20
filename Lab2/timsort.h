#ifndef TIMSORT_H
#define TIMSORT_H

#include <windows.h>

#define MIN_MERGE 64

typedef struct {
    int *array;
    int left;
    int right;
} sort_task_t;


typedef struct {
    sort_task_t *tasks;
    int task_count;
    int task_capacity;
    int next_task_index;
    CRITICAL_SECTION queue_cs;
    CONDITION_VARIABLE queue_cv;
    BOOL shutdown;
} task_queue_t;


typedef struct {
    int thread_id;
    task_queue_t *queue;
} worker_thread_args_t;


void fill_array_random(int arr[], int n);
int is_sorted(int arr[], int n);
double get_time_ms(void);
void print_system_info(void);

void insertion_sort(int arr[], int left, int right);
void merge(int arr[], int l, int m, int r);
int find_next_run(int arr[], int start, int n);
void sequential_timsort(int arr[], int n);
void parallel_timsort(int arr[], int n, int num_threads);

void task_queue_init(task_queue_t *queue, int capacity);
void task_queue_destroy(task_queue_t *queue);
void task_queue_add_task(task_queue_t *queue, int *array, int left, int right);
BOOL task_queue_get_task(task_queue_t *queue, sort_task_t *task);

DWORD WINAPI worker_thread(LPVOID arg);

#endif