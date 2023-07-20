#include <bits/pthreadtypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <sys/sysinfo.h>
#include <time.h>

#define THREAD_NUM 4

sem_t semRead;
sem_t semAnalyze;
sem_t semPrint;

bool g_terminate = true;
double cpu_percentage = 0.0;
float readTime;

typedef struct
{
    char cpu_name[255];
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
} CPU_stats;
static CPU_stats g_st;

void *Reader(void *args)
{
    args = args;
    // Can read choosen CPU, needs to initialize CPU_stats st[get_nprocs_conf]
    int cpu_num = get_nprocs();
    int cpu_num_conf = get_nprocs_conf();
    clock_t t_start;
    clock_t t_end;

    while (1)
    {
        sem_wait(&semRead);
        g_terminate = false;
        t_start = clock();
        FILE *fp = fopen("/proc/stat", "r");
        if (fp == NULL)
        {
            perror("Unable to read /proc/stat\n");
            pthread_exit(0);
        }

        int cnt = 0; // skipping n cpus
        char ch;
        while ((cnt < 0) && ((ch = getc(fp)) != EOF))
        {
            if (ch == '\n')
                cnt++;
        }

        fscanf(fp, "%s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu", g_st.cpu_name,
               &g_st.user_stat, &g_st.nice_stat, &g_st.system_stat, &g_st.idle_stat,
               &g_st.iowait_stat, &g_st.irq_stat, &g_st.softirq_stat, &g_st.steal_stat,
               &g_st.guest_stat, &g_st.guestnice_stat);
        fclose(fp);

        sem_post(&semAnalyze);

        t_end = clock();
        readTime = (double)(t_end - t_start) / CLOCKS_PER_SEC;
        unsigned int timetosleep = (unsigned int)((1.0 - readTime) * 1000000);
        printf("Reader time execution %d\n", timetosleep);
        usleep(timetosleep);
    }

    return NULL;
}

void *Printer(void *args)
{
    args = args;
    sleep(1);
    printf("CPU_NUM: %d %d\n", get_nprocs(), get_nprocs_conf());
    printf("CPU stats: %s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu\n", g_st.cpu_name,
           g_st.user_stat, g_st.nice_stat, g_st.system_stat, g_st.idle_stat,
           g_st.iowait_stat, g_st.irq_stat, g_st.softirq_stat, g_st.steal_stat,
           g_st.guest_stat, g_st.guestnice_stat);

    while (1)
    {
        sem_wait(&semPrint);
        printf("%s percentage usage: %f\n", g_st.cpu_name, cpu_percentage);
        sem_post(&semRead);
    }

    return NULL;
}
void *Analyzer(void *args)
{
    // sleep(2);
    args = args;
    bool turn = true;
    int usertime, nicetime = 0;
    unsigned long long int totaltime_t, idlealltime_t, idlealltime, systemalltime, virtualtime, totaltime = 0;
    double totald, idled = 0.0;

    while (1)
    {
        sem_wait(&semAnalyze);
        usertime = g_st.user_stat - g_st.guest_stat;
        nicetime = g_st.nice_stat - g_st.guestnice_stat;
        systemalltime = g_st.system_stat + g_st.irq_stat + g_st.softirq_stat;
        virtualtime = g_st.guest_stat + g_st.nice_stat;
        if (turn)
        {
            idlealltime = g_st.idle_stat + g_st.iowait_stat;
            totaltime = usertime + nicetime + systemalltime + idlealltime + g_st.steal_stat + virtualtime;
            totald = totaltime - totaltime_t;
            idled = idlealltime - idlealltime_t;
            cpu_percentage = (totald - idled) / totald * 100.0;
        }
        else
        {
            idlealltime_t = g_st.idle_stat + g_st.iowait_stat;
            totaltime_t = usertime + nicetime + systemalltime + idlealltime_t + g_st.steal_stat + virtualtime;
            totald = totaltime_t - totaltime;
            idled = idlealltime_t - idlealltime;
            cpu_percentage = (totald - idled) / totald * 100.0;
        }
        turn = !turn;
        sem_post(&semPrint);
    }
    return NULL;
}

void *Watchdog(void *args)
{
    args = args;
    while (1)
    {
        sleep(2);
        if (g_terminate)
        {
            sem_destroy(&semPrint);
            sem_destroy(&semRead);
            sem_destroy(&semAnalyze);
            exit(0);
        }
        g_terminate = true;
    }
    return NULL;
}

int main()
{
    pthread_t th[THREAD_NUM];
    sem_init(&semRead, 0, 1);
    sem_init(&semAnalyze, 0, 0);
    sem_init(&semPrint, 0, 0);

    if (pthread_create(&th[0], NULL, &Reader, NULL) != 0)
    {
        perror("Failed to create thread\n");
    }
    if (pthread_create(&th[1], NULL, &Printer, NULL) != 0)
    {
        perror("Failed to create thread\n");
    }
    if (pthread_create(&th[2], NULL, &Analyzer, NULL) != 0)
    {
        perror("Failed to create thread\n");
    }
    if (pthread_create(&th[3], NULL, &Watchdog, NULL) != 0)
    {
        perror("Failed to create thread\n");
    }

    for (int i = 0; i < THREAD_NUM; i++)
    {
        if (pthread_join(th[i], NULL) != 0)
        {
            perror("Failed to join thread\n");
        }
    }

    sem_destroy(&semPrint);
    sem_destroy(&semRead);
    sem_destroy(&semAnalyze);

    return 0;
}
