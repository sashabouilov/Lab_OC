#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <time.h>
#include <string.h>
#include "timsort.h"


void fill_array_random(int arr[], int n) {
    for (int i = 0; i < n; i++) {
        arr[i] = rand() % 1000000;
    }
}


int is_sorted(int arr[], int n) {
    for (int i = 0; i < n - 1; i++) {
        if (arr[i] > arr[i + 1]) {
            return 0;
        }
    }
    return 1;
}


double get_time_ms(void) {
    static LARGE_INTEGER frequency = {0};
    if (frequency.QuadPart == 0) {
        QueryPerformanceFrequency(&frequency);
    }
    
    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);
    return (double)time.QuadPart / (double)frequency.QuadPart * 1000.0;
}


void print_system_info(void) {
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    
    printf("System Information\n");
    printf("Number of processors: %lu\n", sys_info.dwNumberOfProcessors);
    printf("Page size: %lu bytes\n", sys_info.dwPageSize);
    printf("Architecture: ");
    
    switch (sys_info.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_AMD64:
            printf("x64\n");
            break;
        case PROCESSOR_ARCHITECTURE_ARM:
            printf("ARM\n");
            break;
        case PROCESSOR_ARCHITECTURE_ARM64:
            printf("ARM64\n");
            break;
        case PROCESSOR_ARCHITECTURE_INTEL:
            printf("x86\n");
            break;
        default:
            printf("Unknown\n");
    }
    printf("\n\n");
}


void insertion_sort(int arr[], int left, int right) {
    for (int i = left + 1; i <= right; i++) {
        int key = arr[i];
        int j = i - 1;
        while (j >= left && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}


void merge(int arr[], int l, int m, int r) {
    int len1 = m - l + 1, len2 = r - m;
    int *left = (int *)malloc(len1 * sizeof(int));
    int *right = (int *)malloc(len2 * sizeof(int));

    if (!left || !right) {
        fprintf(stderr, "Memory allocation error during merge\n");
        exit(1);
    }

    for (int i = 0; i < len1; i++) 
        left[i] = arr[l + i];
    for (int i = 0; i < len2; i++) 
        right[i] = arr[m + 1 + i];

    int i = 0, j = 0, k = l;
    while (i < len1 && j < len2) {
        if (left[i] <= right[j]) {
            arr[k] = left[i];
            i++;
        } else {
            arr[k] = right[j];
            j++;
        }
        k++;
    }

    while (i < len1) {
        arr[k] = left[i];
        i++;
        k++;
    }
    while (j < len2) {
        arr[k] = right[j];
        j++;
        k++;
    }

    free(left);
    free(right);
}


int find_next_run(int arr[], int start, int n) {
    if (start == n - 1) return n;
    
    int i = start;
    
    if (arr[i] <= arr[i + 1]) {
        while (i < n - 1 && arr[i] <= arr[i + 1]) i++;
    } else { 
        while (i < n - 1 && arr[i] > arr[i + 1]) i++;
        
        int l = start, r = i;
        while (l < r) {
            int temp = arr[l];
            arr[l] = arr[r];
            arr[r] = temp;
            l++;
            r--;
        }
    }
    return i + 1;
}


void sequential_timsort(int arr[], int n) {
    if (n <= 1) return;

    int i = 0;
    while (i < n) {
        int run_end = find_next_run(arr, i, n);
        
        if (run_end - i < MIN_MERGE) {
            run_end = (i + MIN_MERGE < n) ? i + MIN_MERGE : n;
        }
        insertion_sort(arr, i, run_end - 1);
        i = run_end;
    }

    int size = MIN_MERGE;
    while (size < n) {
        for (int left = 0; left < n; left += 2 * size) {
            int mid = left + size - 1;
            if (mid >= n - 1) break;
            
            int right = (left + 2 * size - 1) < (n - 1) ? 
                       (left + 2 * size - 1) : (n - 1);
            
            merge(arr, left, mid, right);
        }
        size *= 2;
    }
}


void task_queue_init(task_queue_t *queue, int capacity) {
    queue->tasks = (sort_task_t *)malloc(capacity * sizeof(sort_task_t));
    if (!queue->tasks) {
        fprintf(stderr, "Error allocating memory for task queue\n");
        exit(1);
    }
    
    queue->task_capacity = capacity;
    queue->task_count = 0;
    queue->next_task_index = 0;
    queue->shutdown = FALSE;
    
    InitializeCriticalSection(&queue->queue_cs);
    InitializeConditionVariable(&queue->queue_cv);
}


void task_queue_destroy(task_queue_t *queue) {
    free(queue->tasks);
    DeleteCriticalSection(&queue->queue_cs);
}


void task_queue_add_task(task_queue_t *queue, int *array, int left, int right) {
    EnterCriticalSection(&queue->queue_cs);
    
    if (queue->task_count >= queue->task_capacity) {
        queue->task_capacity *= 2;
        sort_task_t *new_tasks = (sort_task_t *)realloc(
            queue->tasks, queue->task_capacity * sizeof(sort_task_t));
        if (!new_tasks) {
            fprintf(stderr, "Error reallocating memory for task queue\n");
            exit(1);
        }
        queue->tasks = new_tasks;
    }
    
    int index = queue->task_count;
    queue->tasks[index].array = array;
    queue->tasks[index].left = left;
    queue->tasks[index].right = right;
    queue->task_count++;
    
    WakeConditionVariable(&queue->queue_cv);
    LeaveCriticalSection(&queue->queue_cs);
}


BOOL task_queue_get_task(task_queue_t *queue, sort_task_t *task) {
    EnterCriticalSection(&queue->queue_cs);
    
    if (queue->shutdown && queue->next_task_index >= queue->task_count) {
        LeaveCriticalSection(&queue->queue_cs);
        return FALSE;
    }
    
    while (queue->next_task_index >= queue->task_count && !queue->shutdown) {
        SleepConditionVariableCS(&queue->queue_cv, &queue->queue_cs, INFINITE);
    }
    
    if (queue->next_task_index >= queue->task_count) {
        LeaveCriticalSection(&queue->queue_cs);
        return FALSE;
    }
    
    int task_index = queue->next_task_index;
    *task = queue->tasks[task_index];
    queue->next_task_index++;
    
    LeaveCriticalSection(&queue->queue_cs);
    return TRUE;
}


DWORD WINAPI worker_thread(LPVOID arg) {
    worker_thread_args_t *args = (worker_thread_args_t *)arg;
    task_queue_t *queue = args->queue;

    sort_task_t task;
    while (task_queue_get_task(queue, &task)) {
        insertion_sort(task.array, task.left, task.right);
    }
    
    return 0;
}


void parallel_timsort(int arr[], int n, int num_threads) {
    if (n <= 1 || num_threads <= 1) {
        sequential_timsort(arr, n);
        return;
    }

    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    int recommended_limit = (int)sys_info.dwNumberOfProcessors * 2;
    if (num_threads > recommended_limit) {
        printf("Warning: Thread count (%d) exceeds recommended limit (%d)\n", 
               num_threads, recommended_limit);
        printf("Reducing to %d threads\n", recommended_limit);
        num_threads = recommended_limit;
    }

    if (n < 10000) {
        sequential_timsort(arr, n);
        return;
    }

    task_queue_t queue;
    task_queue_init(&queue, n);

    HANDLE *threads = (HANDLE *)malloc((num_threads - 1) * sizeof(HANDLE));
    worker_thread_args_t *thread_args = (worker_thread_args_t *)malloc(
        (num_threads - 1) * sizeof(worker_thread_args_t));

    for (int i = 0; i < num_threads - 1; i++) {
        thread_args[i].thread_id = i;
        thread_args[i].queue = &queue;
        threads[i] = CreateThread(NULL, 0, worker_thread, &thread_args[i], 0, NULL);
        if (threads[i] == NULL) {
            fprintf(stderr, "Error creating thread %d\n", i);
            exit(1);
        }
    }

    int i = 0;
    while (i < n) {
        int run_end = find_next_run(arr, i, n);
        task_queue_add_task(&queue, arr, i, run_end - 1);
        i = run_end;
    }

    while (1) {
        EnterCriticalSection(&queue.queue_cs);
        BOOL all_tasks_processed = (queue.next_task_index >= queue.task_count);
        LeaveCriticalSection(&queue.queue_cs);
        
        if (all_tasks_processed) {
            Sleep(10);
            EnterCriticalSection(&queue.queue_cs);
            all_tasks_processed = (queue.next_task_index >= queue.task_count);
            LeaveCriticalSection(&queue.queue_cs);
            if (all_tasks_processed) break;
        }
        Sleep(1);
    }

    EnterCriticalSection(&queue.queue_cs);
    queue.shutdown = TRUE;
    WakeAllConditionVariable(&queue.queue_cv);
    LeaveCriticalSection(&queue.queue_cs);

    WaitForMultipleObjects(num_threads - 1, threads, TRUE, INFINITE);

    for (int i = 0; i < num_threads - 1; i++) {
        CloseHandle(threads[i]);
    }

    int *run_ends = (int *)malloc(n * sizeof(int));
    int run_count = 0;
    
    i = 0;
    while (i < n) {
        int run_end = find_next_run(arr, i, n);
        run_ends[run_count++] = run_end;
        i = run_end;
    }

    while (run_count > 1) {
        int new_run_count = 0;
        
        for (int i = 0; i < run_count; i += 2) {
            if (i + 1 >= run_count) {
                run_ends[new_run_count++] = run_ends[i];
                continue;
            }
            
            int left_start = (i == 0) ? 0 : run_ends[i - 1];
            int left_end = run_ends[i] - 1;
            int right_start = run_ends[i];
            int right_end = run_ends[i + 1] - 1;
            
            merge(arr, left_start, left_end, right_end);
            
            run_ends[new_run_count++] = run_ends[i + 1];
        }
        
        run_count = new_run_count;
    }

    free(run_ends);
    free(threads);
    free(thread_args);
    task_queue_destroy(&queue);
}