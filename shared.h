
#ifndef CS342PR2_SHARED_H
#define CS342PR2_SHARED_H

#include <semaphore.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>

#define BUFSIZE 100
#define MAXWORDNAME 128
#define MAXLINE 1024
#define N 10 //Max # of clients
#define IN 0
#define OUT 1
#define SEM_PERMISSIONS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)

char shm_name[MAXWORDNAME];
char sem_name[N + 1][MAXWORDNAME + 2];

struct request {
    char keyword[MAXWORDNAME];
    int queueIndex;
};

struct shared {
    int result_queue_state[N];
    int resultQueues[N][BUFSIZE];
    int inout[N][2];
    struct request requestQueue[N];
    int requestIn;
    int requestOut;
};
#endif //CS342PR2_SHARED_H