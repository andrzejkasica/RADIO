#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>

#define THREAD_NUM 4

int buffer[10];
int count = 0;

typedef struct{
    unsigned long user_stat;
    unsigned long nice_stat;
    unsigned long system_stat;
    unsigned long idle_stat;
    unsigned long iowait_stat;
    unsigned long irq_stat;
    unsigned long softirq_stat;
}CPU_stats;


void* Reader(void* args) {
    
    return NULL;
}
void* Printer(void* args) {
    CPU_stats* st = (CPU_stats*)args;
    printf("CPU stats: %lu %lu %lu %lu %lu %lu %lu\n", (*st).user_stat, (*st).nice_stat, (*st).system_stat, (*st).idle_stat,
            (*st).iowait_stat, (*st).irq_stat, (*st).softirq_stat);
    return NULL;
}
void* Analyzer(void* args) {
    return NULL;
}
void* Watchdog(void* args) {
    return NULL;
}

int main() {
    pthread_t th[THREAD_NUM];
    CPU_stats* my_st = malloc(sizeof(CPU_stats));
    if(my_st == NULL){
        perror("Memory allocation failed");
        return 1;
    }


    if (pthread_create(&th[0], NULL, &Reader, NULL) != 0) {
         perror("Failed to create thread\n");
    }
    if (pthread_create(&th[1], NULL, &Printer, (void*)my_st) != 0) {
         perror("Failed to create thread\n"); 
    }
    if (pthread_create(&th[2], NULL, &Analyzer, NULL) != 0) {
         perror("Failed to create thread\n"); 
    }
    if (pthread_create(&th[3], NULL, &Watchdog, NULL) != 0) {
         perror("Failed to create thread\n"); 
    }


    for (int i = 0; i < THREAD_NUM; i++) {
        if (pthread_join(th[i], NULL) != 0) {
            perror("Failed to join thread\n");
        }
    }
    free(my_st);
    return 0;
}

