#include "shared.h"

/*
 * Lengths include the newline or null character at the end
 */

#define SHM_SIZE sizeof (struct shared)

void *keywordSearchThread(void *arg);

struct param {
    char file_name[MAXWORDNAME];
    struct request *req;
    struct shared *sdp;
    sem_t *sem_resultq;
    sem_t *sem_n;
};

int main(int argc, char *argv[]) {
    sem_t *sem[N + 1];
    pthread_t thread_ids[N];
    int fd, i, ret;
    struct stat sbuf;
    void *shm_start;
    struct shared *sdp;
    struct param *par;
    par = (struct param *) malloc(sizeof(struct param));
    par->req = (struct request *) malloc(sizeof(struct request));
    par->sdp = (struct shared *) malloc(sizeof(struct shared));

    if (argc != 4) {
        printf("Input format where f is fileName and k is keyword: server <shm_name> <inputfilename> <sem_name>");
        exit(1);
    }

    //Retrieve Arguments
    for (i = 1; i < argc; i++) {
        if (strlen(argv[i]) > MAXWORDNAME || strlen(argv[i]) == 0) {
            printf("One of the inputs is incorrect. Exiting...\n");
            return 1;
        }
    }

    strcpy(shm_name, argv[1]);
    strcpy(par->file_name, argv[2]);
    //Create and init semaphores
    for (i = 0; i < N + 1; i++) {
        char semNo = i + '0';
        strcpy(sem_name[i], argv[3]);
        strcat(sem_name[i], &semNo); //IF THAT CASTING WONT WORK USE SPRINTF
        sem_unlink(sem_name[i]);

        sem[i] = sem_open(sem_name[i], O_RDWR | O_CREAT, 0660, 1); //might add O_RDWR
        if (sem[i] < 0) {
            printf("Cannot create semaphore%d", i);
            exit(1);
        }
        //printf("sem %d created\n", i);
    }

    //Init shared memory
    shm_unlink(shm_name);

    fd = shm_open(shm_name, O_RDWR | O_CREAT, 0660);
    if (fd < 0) {
        perror("can not create shared memory\n");
        exit(1);
    }
    //printf("shm created, fd = %d\n", fd);

    ftruncate(fd, SHM_SIZE);
    /* set size of shared memory */
    fstat(fd, &sbuf);
    //printf("shm_size=%d\n", (int) sbuf.st_size);

    shm_start = mmap(NULL, sbuf.st_size, PROT_READ | PROT_WRITE,
                     MAP_SHARED, fd, 0);

    if (shm_start < 0) {
        perror("Cannot map shm\n");
        exit(1);
    }
    //printf("mapped shm; start_address=%u\n", (unsigned int) shm_start);
    close(fd);

    sdp = (struct shared *) shm_start;

    for (i = 0; i < N; ++i) {
        int j, k;
        thread_ids[i] = (pthread_t) -1;
        sdp->inout[i][IN] = 0;
        sdp->inout[i][OUT] = 0;
        sdp->result_queue_state[i] = 0;
        sdp->requestQueue[i].queueIndex = 0;
        for (j = 0; j < MAXWORDNAME - 1; j++)
            sdp->requestQueue->keyword[j] = ' ';
        sdp->requestQueue->keyword[MAXWORDNAME - 1] = '\0';
        for (k = 0; k < BUFSIZE; k++)
            sdp->resultQueues[i][k] = 0;
    }
    sdp->requestIn = 0;
    sdp->requestOut = 0;

    int qindex = 0;

    //printf("Shared memory initialized!\n");

    while (1) {
        fflush(stdout);
        //Wait for a request to arrive
        while (sdp->requestIn == sdp->requestOut) {
            //printf("request queue is empty\n");
        }

        //Retrieve request from request queue & Create a Thread
        sem_wait(sem[N]); //hold request queue
        par->req->queueIndex = sdp->requestQueue[sdp->requestOut].queueIndex;
        //strcpy(par->req->keyword, "one");
        strcpy(par->req->keyword, sdp->requestQueue[sdp->requestOut].keyword);
        par->sdp = sdp;
        par->sem_resultq = sem[sdp->requestQueue[sdp->requestOut].queueIndex];
        par->sem_n = sem[N];
        //sdp->requestQueue[sdp->requestOut].queueIndex = -1;
        strcpy(sdp->requestQueue[sdp->requestOut].keyword, "");
        sdp->requestOut = (sdp->requestOut + 1) % N;
        sem_post(sem[N]); //release request queue

        ret = pthread_create(&(thread_ids[qindex]), NULL, keywordSearchThread, (void *) par);
        if (ret != 0) {
            printf("thread create failed \n");
            exit(1);
        }
        //printf("thread created \n");
        //ret = pthread_join(thread_ids[qindex], NULL);
        fflush(stdout);
    }
}

// TODO WATCH THREAD EXIT CONDITIONS
// TODO CHECK DATA
void *keywordSearchThread(void *arg) {
    struct param *p;
    FILE *file;
    char *str; //[MAX_LINE_LENGTH];
    int lineNo = 1;
    int qindex;

    p = (struct param *) arg;

    qindex = p->req->queueIndex;

    //printf("qindex %d started.\n", qindex);
    //printf("Keyword : %s \n", p->req->keyword);


    //Check Constraints
    if (strlen(p->req->keyword) == 1) {
        printf("Error: Keyword is null.");
        pthread_exit(NULL);
    } else if (strlen(p->req->keyword) > MAXWORDNAME) {
        printf("Error: Keyword exceeds max length (128).");
        pthread_exit(NULL);
    }

    if (strlen(p->file_name) == 1) {
        printf("Error: File name is null.");
        pthread_exit(NULL);

    } else if (strlen(p->file_name) > MAXWORDNAME) {
        printf("Error: File name exceeds max length (1024).");
        pthread_exit(NULL);
    }

    //Try to open file
    file = fopen(p->file_name, "r");
    if (file == NULL) {
        //printf("Error: Couldn't open file.\n");
        pthread_exit(NULL);
    }

    str = malloc(MAXLINE * sizeof(char));
    while (fgets(str, MAXLINE, file) != NULL) { //fgets() reads a line
        //printf("Read line %d\n", lineNo);
        if (strstr(str, p->req->keyword) != NULL) { //strstr() returns a pointer to the first occurrence of keyword
             //hold result queue of qindex
            //TODO: Do we need to put this inside a do-while loop and add semaphore of i'th result queue ??
            while ((p->sdp->inout[qindex][IN] + 1) % BUFSIZE == p->sdp->inout[qindex][OUT]) {
                //printf("Result queue %d is full", qindex);
            }
            //printf("Wait finished");
            sem_wait(p->sem_resultq);

            p->sdp->resultQueues[qindex][p->sdp->inout[qindex][IN]] = lineNo;
            p->sdp->inout[qindex][IN] = (p->sdp->inout[qindex][IN] + 1) % BUFSIZE;
            sem_post(p->sem_resultq); //release result queue of qindex
            //printf("Line: %d.\n", lineNo);
            //found = 1;
        }
        lineNo++;
    }

    //Add a -1 to the end
    sem_wait(p->sem_resultq); //hold result queue of qindex
    p->sdp->resultQueues[qindex][p->sdp->inout[qindex][IN]] = -1;
    p->sdp->inout[qindex][IN] = (p->sdp->inout[qindex][IN] + 1) % BUFSIZE;
    sem_post(p->sem_resultq); //release result queue of qindex

    fclose(file); //TODO: DIDN'T CHECK if file is already closed. Might cause problem.
    //printf("thread exiting...\n");
    //TODO:free str space
    //p->sdp->requestQueue[qindex].queueIndex = -1;
    sem_wait(p->sem_n);
    memset(p->sdp->requestQueue[qindex].keyword, 0, sizeof(p->sdp->requestQueue[qindex].keyword));
    p->sdp->requestIn = 0;
    p->sdp->requestOut = 0;
    sem_post(p->sem_n);
    free(str);
    str = NULL;
    fflush(stdout);
    pthread_exit(NULL);
}