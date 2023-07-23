// defines preventing usleep warning
#define _XOPEN_SOURCE 600
#define _POSIX_C_SOURCE 200112L

#include <unistd.h>
#include <bits/pthreadtypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <semaphore.h>
#include <sys/sysinfo.h>
#include <time.h>
#include <signal.h>
#include <string.h>

#define THREAD_NUM 4

static sem_t semRead;
static sem_t semAnalyze;
static sem_t semPrint;
static pthread_mutex_t mutexRead = PTHREAD_MUTEX_INITIALIZER;

static volatile sig_atomic_t done = 0;

// global variable for programme termination -> watchdog Thread
static bool g_terminate = true;
static double cpu_percentage[9] = {0.0};
static double readTime = 0.0;

// global variables for logger

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

struct CPU_numbers
{
    int cpu_numb;
    int cpu_numb_conf;
};

static CPU_stats g_st[9];
static struct CPU_numbers g_nb;

static void term(int signum)
{
    signum = signum;
    done = 1;
}

void *Reader(void *args)
{
    args = args;
    // vars for read time thread
    clock_t t_start;
    clock_t t_end;
    // additional variables for proc/stat line skipping
    int cpu_cnt = 0;
    int cnt = 0;
    char ch;

    while (!done)
    {
        sem_wait(&semRead);
        g_terminate = false;
        cpu_cnt = 0;
        t_start = clock();
        FILE *fp = fopen("/proc/stat", "r");
        if (fp == NULL)
        {
            perror("Unable to read /proc/stat\n");
            pthread_exit(0);
        }

        while (cpu_cnt < g_nb.cpu_numb_conf + 1)
        {
            fscanf(fp, "%s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu", g_st[cpu_cnt].cpu_name,
                   &g_st[cpu_cnt].user_stat, &g_st[cpu_cnt].nice_stat, &g_st[cpu_cnt].system_stat, &g_st[cpu_cnt].idle_stat,
                   &g_st[cpu_cnt].iowait_stat, &g_st[cpu_cnt].irq_stat, &g_st[cpu_cnt].softirq_stat, &g_st[cpu_cnt].steal_stat,
                   &g_st[cpu_cnt].guest_stat, &g_st[cpu_cnt].guestnice_stat);
            while ((cnt < 1) && ((ch = (char)getc(fp)) != EOF)) // skip a line
            {
                if (ch == '\n')
                {
                    cnt++;
                }
            }
            cpu_cnt++;
            cnt = 0;
        }
        fclose(fp);
        sem_post(&semAnalyze);
        t_end = clock();
        readTime = (double)(t_end - t_start) / CLOCKS_PER_SEC;
        unsigned int timetosleep = (unsigned int)((1.0 - readTime) * 1000000);
        // printf("Reader time execution %d\n", timetosleep);
        usleep(timetosleep);
    }
    pthread_exit(NULL);
}

void *Printer(void *args)
{
    args = args;
    unsigned int cnt = 0;
    sleep(1); //  in future some flag indicator will be added
    printf("CPU stats: %s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu\n", g_st[cnt].cpu_name,
           g_st[cnt].user_stat, g_st[cnt].nice_stat, g_st[cnt].system_stat, g_st[cnt].idle_stat,
           g_st[cnt].iowait_stat, g_st[cnt].irq_stat, g_st[cnt].softirq_stat, g_st[cnt].steal_stat,
           g_st[cnt].guest_stat, g_st[cnt].guestnice_stat);

    while (!done)
    {
        sem_wait(&semPrint);
        pthread_mutex_lock(&mutexRead);
        cnt = 0; // comment this line and uncomment for to print each cpu usage
        // for (cnt = 0; cnt < g_nb.cpu_numb_conf + 1; cnt++)
        //{
        printf("%s perc: %f\n", g_st[cnt].cpu_name, cpu_percentage[cnt]);
        //}
        // printf("Procces ID:%d\n",getpid());
        pthread_mutex_unlock(&mutexRead);
        sem_post(&semRead);
    }
    pthread_exit(NULL);
}

void *Analyzer(void *args)
{
    args = args;
    bool turn = true;
    // variables for cpu[0- cpus total] percentage count
    int cpus_total = g_nb.cpu_numb + 1; // initialize with total num of cpus, count with avaivable
    unsigned long usertime[cpus_total], nicetime[cpus_total];
    unsigned long totaltime_t[cpus_total], idlealltime_t[cpus_total], idlealltime[cpus_total], systemalltime[cpus_total], virtualtime[cpus_total], totaltime[cpus_total];
    double totald[cpus_total], idled[cpus_total];

    int cnt = 0;
    while (!done)
    {
        sem_wait(&semAnalyze);
        cnt = 0;
        while (cnt < g_nb.cpu_numb_conf + 1)
        {
            usertime[cnt] = g_st[cnt].user_stat - g_st[cnt].guest_stat;
            nicetime[cnt] = g_st[cnt].nice_stat - g_st[cnt].guestnice_stat;
            systemalltime[cnt] = g_st[cnt].system_stat + g_st[cnt].irq_stat + g_st[cnt].softirq_stat;
            virtualtime[cnt] = g_st[cnt].guest_stat + g_st[cnt].nice_stat;
            if (turn)
            {
                idlealltime[cnt] = g_st[cnt].idle_stat + g_st[cnt].iowait_stat;
                totaltime[cnt] = usertime[cnt] + nicetime[cnt] + systemalltime[cnt] + idlealltime[cnt] + g_st[cnt].steal_stat + virtualtime[cnt];
                totald[cnt] = (double)(totaltime[cnt] - totaltime_t[cnt]);
                idled[cnt] = (double)(idlealltime[cnt] - idlealltime_t[cnt]);
                cpu_percentage[cnt] = (totald[cnt] - idled[cnt]) / totald[cnt] * 100.0;
            }
            else
            {
                idlealltime_t[cnt] = g_st[cnt].idle_stat + g_st[cnt].iowait_stat;
                totaltime_t[cnt] = usertime[cnt] + nicetime[cnt] + systemalltime[cnt] + idlealltime_t[cnt] + g_st[cnt].steal_stat + virtualtime[cnt];
                totald[cnt] = (double)(totaltime_t[cnt] - totaltime[cnt]);
                idled[cnt] = (double)(idlealltime_t[cnt] - idlealltime[cnt]);
                cpu_percentage[cnt] = (totald[cnt] - idled[cnt]) / totald[cnt] * 100.0;
            }
            cnt++;
        }
        turn = !turn;
        sem_post(&semPrint);
    }
    pthread_exit(NULL);
}

void *Watchdog(void *args)
{
    args = args;
    while (!done)
    {
        sleep(2);
        if (g_terminate)
        {
            sem_destroy(&semPrint);
            sem_destroy(&semRead);
            sem_destroy(&semAnalyze);
            pthread_mutex_destroy(&mutexRead);
            exit(0);
        }
        g_terminate = true;
    }
    sem_destroy(&semPrint);
    sem_destroy(&semRead);
    sem_destroy(&semAnalyze);
    pthread_mutex_destroy(&mutexRead);
    exit(0);
}

int main()
{
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = term;
    sigaction(SIGINT, &action, NULL);

    // memset(g_st, 0, sizeof(g_st)); //check if make diff in valgrind
    // memset(&g_nb, 0, sizeof(g_nb));

    pthread_t th[THREAD_NUM];
    sem_init(&semRead, 0, 1);
    sem_init(&semAnalyze, 0, 0);
    sem_init(&semPrint, 0, 0);
    // number of cpus & number of cpus confirmed to use
    g_nb.cpu_numb = get_nprocs();
    g_nb.cpu_numb_conf = get_nprocs_conf();

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

    // sem_destroy(&semPrint);
    // sem_destroy(&semRead);
    // sem_destroy(&semAnalyze);
    // pthread_mutex_destroy(&mutexRead);

    return 0;
}
