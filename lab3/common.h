#ifndef COMMON_H
#define COMMON_H

#define _POSIX_C_SOURCE 200809L
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include "recv_buf.h"

#define BUF_SIZE 1048576    /* 1024*1024 = 1M */
#define MAX_STRIPS 50       /* Total num of image strips */
#define ECE252_HEADER "X-Ece252-Fragment: "

/* To indicate if the strip has been downloaded */
int is_done[50] = {0};

typedef struct{
    char *uncomp_image;
    size_t max_size;
    size_t size;
    recv_buf *buf;
    int capacity;
    int num;

    int in;                 /* index for producer */
    int out;                /* index for consumer */
    sem_t empty;            /* semaphore for empty slots */
    sem_t full;             /* semaphore for full slots */
}CircularQueue;

void init_image_queue(CircularQueue *image_queue, int buffer_size);
void cleanup_image_queue(CircularQueue *image_queue);

#endif