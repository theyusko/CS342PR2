#include "shared.h"

// TODO WAIT ON THREAD
// TODO WAIT ON RESULTS
// TODO MAKE SURE ITS THE SAME REQUEST
int main(int argc, char *argv[]) {
    sem_t *sem[N + 1];
    char keyword[MAXWORDNAME];
    int fd, i, j = 0;
    struct stat sbuf;
    void *shm_start;
    struct shared *sdp;
    if (argc != 4) {
        printf("Input format where f is fileName and k is keyword: server <shm_name> <inputfilename> <sem_name>");
        exit(1);
    }

    //Retrieve Arguments
    strcpy(shm_name, argv[1]);
    strcpy(keyword, argv[2]);

    //Init semaphores
    for (i = 0; i < N + 1; i++) {
        char semNo = i + '0';
        strcpy(sem_name[i], argv[3]);
        strcat(sem_name[i], &semNo); //IF THAT CASTING WONT WORK USE SPRINTF
        //sem_unlink(sem_name[i]);

        sem[i] = sem_open(sem_name[i], O_RDWR);
        if (sem[i] < 0) {
            printf("Cannot create semaphore%d", i);
            exit(1);
        }
        //printf("sem %d created\n", i);
    }
    for (i = 1; i < argc; i++) {
        if (strlen(argv[i]) > MAXWORDNAME || strlen(argv[i]) == 0) {
            printf("One of the inputs is incorrect. Exiting...\n");
            return 1;
        }
    }

    //Init shared memory
    //shm_unlink(shm_name);

    fd = shm_open(shm_name, O_RDWR, 0600);
    if (fd < 0) {
        perror("can not open shm\n");
        exit(1);
    }
    //printf("shm open success, fd = %d\n", fd);

    fstat(fd, &sbuf);
    //printf("shm size = %d\n", (int) sbuf.st_size);

    shm_start = mmap(NULL, sbuf.st_size, PROT_READ | PROT_WRITE,
                     MAP_SHARED, fd, 0);

    if (shm_start < 0) {
        perror("can not map the shm \n");
        exit(1);
    }
    //printf("mapped shm; start_address=%u\n", (unsigned int) shm_start);
    close(fd);
    sdp = (struct shared *) shm_start;

    //Find an unused result queue if possible
    int done = 0;
    sem_wait(sem[N]);
    for (i = 0; !done; i++) {
        //hold request queue`s semaphore
        done = !(sdp->result_queue_state[i]); //if result queue is empty (0), done is true(1)
        //i++;
        if (i == N) {
            printf("Too many clients started. Exiting...\n");
            sem_post(sem[N]); //release
            return 1;
        }
        if (done) {
            //sem_wait(sem[N]); //hold request queue's semaphore
            sdp->result_queue_state[i] = 1;
            //sem_post(sem[N]); //release

            /*while((sdp->requestIn +1) % N == sdp->requestOut) {
                    // Request Queue is full
            }*/

            //sem_wait(sem[N]);
            //printf("i: %d\n", i);
            sdp->requestQueue[sdp->requestIn].queueIndex = i;
            strcpy(sdp->requestQueue[sdp->requestIn].keyword, keyword);
            sdp->requestIn = (sdp->requestIn + 1) % N;
            
        }
    }
    sem_post(sem[N]);
    i--;

    //printf("i: %d\n", i);

    /*sem_wait(sem[N]); //hold request queue's semaphore
    sdp->result_queue_state[i] = 1;
    //sem_post(sem[N]); //release

*//*    while((sdp->requestIn +1) % N == sdp->requestOut) {
            // Request Queue is full
    }*//*

    //sem_wait(sem[N]);
    sdp->requestQueue[sdp->requestIn].queueIndex = i;
    strcpy(sdp->requestQueue[sdp->requestIn].keyword, keyword);
    sdp->requestIn = (sdp->requestIn + 1) % N;
    sem_post(sem[N]);*/

    /*while(sdp->resultQueues[i][j] != -1) {
        //printf("%d\n", sdp->resultQueues[i][j]);
        j++;
    }*/
    j = 0;
    fflush(stdout);
    while (1) {
        /*int *t;
        while(*t <= 2) {
            sem_post(sem[i]);
            sem_getvalue(sem[i], t);
        }*/
        sem_wait(sem[i]); //hold result queue i's semaphore
        if(sdp->resultQueues[i][j] == -1) {
            sem_post(sem[i]);
            break;
        }
        done = (sdp->inout[i][IN] == sdp->inout[i][OUT]); //if result queue is empty, done
        if (sdp->resultQueues[i][sdp->inout[i][OUT]] != 0 && sdp->resultQueues[i][sdp->inout[i][OUT]] != -1) {
            printf("%d\n", sdp->resultQueues[i][sdp->inout[i][OUT]]);
            sdp->inout[i][OUT] = (sdp->inout[i][OUT] + 1) % BUFSIZE;
        }
        sem_post(sem[i]); //release
        j = (j + 1) % BUFSIZE;
        //if(done)
        //    break;
    }
    fflush(stdout);
    //Client Cleanup
    sem_wait(sem[N]);
    sem_wait(sem[i]);
    for (j = 0; j < BUFSIZE; j++) {
        sdp->resultQueues[i][j] = 0;
    }
    sdp->result_queue_state[i] = 0;
    sdp->inout[i][IN] = 0;
    sdp->inout[i][OUT] = 0;
    sem_post(sem[i]);
    sem_post(sem[N]);
}