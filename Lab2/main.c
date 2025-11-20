#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "timsort.h"


int main(int argc, char *argv[]) {
    print_system_info();
    
    if (argc < 3) {
        printf("Usage: %s <array_size> <max_threads> [seed]\n", argv[0]);
        printf("Example: %s 1000000 4 12345\n", argv[0]);
        return 1;
    }

    int array_size = atoi(argv[1]);
    int max_threads = atoi(argv[2]);
    int seed = (argc > 3) ? atoi(argv[3]) : (int)time(NULL);

    if (array_size <= 0 || max_threads <= 0) {
        printf("Error: array size and thread count must be positive\n");
        return 1;
    }

    srand(seed);
    printf("Execution Parameters:\n");
    printf("  Array size: %d elements\n", array_size);
    printf("  Maximum threads: %d\n", max_threads);
    printf("  Random seed: %d\n", seed);

    int *original_array = (int *)malloc(array_size * sizeof(int));
    if (!original_array) {
        fprintf(stderr, "Error allocating memory for original array\n");
        return 1;
    }
    
    printf("Filling array with random numbers...\n");
    fill_array_random(original_array, array_size);

    printf("\nSequential Version\n");
    int *seq_array = (int *)malloc(array_size * sizeof(int));
    if (!seq_array) {
        fprintf(stderr, "Error allocating memory for sequential array\n");
        return 1;
    }
    memcpy(seq_array, original_array, array_size * sizeof(int));
    
    double start_time = get_time_ms();
    sequential_timsort(seq_array, array_size);
    double seq_time = get_time_ms() - start_time;
    
    printf("Execution time: %.2f ms\n", seq_time);
    printf("Sort verification: %s\n\n", 
           is_sorted(seq_array, array_size) ? "PASSED" : "FAILED");
    free(seq_array);

    printf("Parallel Version\n");
    printf("Threads | Time (ms) | Speedup | Efficiency\n");
    printf("--------+------------+---------+------------\n");

    for (int num_t = 1; num_t <= max_threads; num_t++) {
        printf(">>> NOW TESTING WITH %d THREADS - CHECK TASK MANAGER! <<<\n", num_t);
        
        int *par_array = (int *)malloc(array_size * sizeof(int));
        if (!par_array) {
            fprintf(stderr, "Error allocating memory for parallel array\n");
            return 1;
        }
        memcpy(par_array, original_array, array_size * sizeof(int));

        start_time = get_time_ms();
        parallel_timsort(par_array, array_size, num_t);
        double par_time = get_time_ms() - start_time;

        double speedup = seq_time / par_time;
        double efficiency = (speedup / num_t) * 100.0;

        printf("%7d | %10.2f | %7.2f | %8.1f%% %s\n",
               num_t, par_time, speedup, efficiency,
               is_sorted(par_array, array_size) ? "" : " (SORT ERROR!)");

        free(par_array);
        
        if (num_t < max_threads) {
            printf("Press Enter to continue to next test...");
            getchar();
        }
    }

    free(original_array);
    
    printf("\nProgram completed successfully.\n");
    printf("Final pause - check Task Manager one more time.\n");
    printf("Press Enter to exit...");
    getchar();
    
    return 0;
}