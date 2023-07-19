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
    char    cpu_name[255];
    unsigned long user_stat;
    unsigned long nice_stat;
    unsigned long system_stat;
    unsigned long idle_stat;
    unsigned long iowait_stat;
    unsigned long irq_stat;
    unsigned long softirq_stat;
    unsigned long steal_stat;
    unsigned long guest_stat;
    unsigned long guestnice_stat;
}CPU_stats;


void* Reader(void* args) {
    //Can read choosen CPU, needs to initialize CPU_stats st[get_nprocs_conf]
    int cpu_num = get_nprocs(); 
    int cpu_num_conf = get_nprocs_conf();
    CPU_stats* st = (CPU_stats*)args;
    while(1){
    FILE *fp = fopen("/proc/stat", "r");
    if(fp == NULL){
        perror("Unable to read /proc/stat\n");
        pthread_exit(0);
    }
    char cpun[255];
    int cnt = 0;
    char ch;
    while((cnt < 5) && ((ch = getc(fp)) != EOF))
    {
        if (ch == '\n')
            cnt++;
    }
    fscanf(fp, "%s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",(*st).cpu_name, 
            &(*st).user_stat,   &(*st).nice_stat, &(*st).system_stat,  &(*st).idle_stat,
            &(*st).iowait_stat, &(*st).irq_stat,  &(*st).softirq_stat, &(*st).steal_stat, 
            &(*st).guest_stat,  &(*st).guestnice_stat);
    fclose(fp);
    sleep(1);
    }
    return NULL;
}
void* Printer(void* args) {
    CPU_stats* st = (CPU_stats*)args;
    sleep(1);
    printf("CPU_NUM: %d %d\n", get_nprocs(), get_nprocs_conf());
    printf("CPU stats: %s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu\n",(*st).cpu_name, 
            (*st).user_stat, (*st).nice_stat, (*st).system_stat, (*st).idle_stat,
            (*st).iowait_stat, (*st).irq_stat, (*st).softirq_stat, (*st).steal_stat, 
            (*st).guest_stat,  (*st).guestnice_stat);
    return NULL;
}
void* Analyzer(void* args) {
    sleep(2);
    CPU_stats* st = (CPU_stats*)args;
   while(1){
    int previdle = 0; //prev_idle + prev iowait
    int prevnonidle = 0;
    int prevtotal = 0;
    int idle = (*st).idle_stat + (*st).iowait_stat;
    int nonidle = (*st).user_stat + (*st).nice_stat + (*st).system_stat + (*st).irq_stat + (*st).softirq_stat + (*st).steal_stat;
    int total = idle + nonidle;
    
    double totald = total - prevtotal;
    double idled = idle - previdle;

    double cpu_percentage = (totald - idled)/totald;
    printf("Cpu percentage usage: %f\n", cpu_percentage);
    sleep(1);
   }
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


    if (pthread_create(&th[0], NULL, &Reader,  (void*)my_st) != 0) {
         perror("Failed to create thread\n");
    }
    if (pthread_create(&th[1], NULL, &Printer, (void*)my_st) != 0) {
         perror("Failed to create thread\n"); 
    }
    if (pthread_create(&th[2], NULL, &Analyzer, (void*)my_st) != 0) {
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

