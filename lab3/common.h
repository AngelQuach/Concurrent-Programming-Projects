#ifndef COMMON_H
#define COMMON_H

#define _POSIX_C_SOURCE 200809L
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <semaphore.h>
#include "recv_buf.h"

#define BUF_SIZE 1048576    /* 1024*1024 = 1M */
#define MAX_STRIPS 50       /* Total num of image strips */

/* To indicate if the strip has been downloaded */
int last_seq = 0;
/* The semaphore to be shared among producers/consumers */
sem_t prod_sem;
sem_t cons_sem;

typedef struct{
    char *uncomp_image;
    size_t max_size;
    size_t size;
    recv_buf *buf;
    int capacity;
    int num;

    int in;                 /* index for producer */
    int out;                /* index for consumer */
    sem_t access_sem;       /* semaphore for accessing the last sequence downloaded */
    sem_t empty;            /* semaphore for empty slots */
    sem_t full;             /* semaphore for full slots */
}CircularQueue;

void init_semaphores(CircularQueue *image_queue, int buffer_size);
void init_image_queue(CircularQueue *image_queue, int buffer_size);
void cleanup_image_queue(CircularQueue *image_queue);

void producer_process(CircularQueue *queue, int machine_num, int img_num);

#endif