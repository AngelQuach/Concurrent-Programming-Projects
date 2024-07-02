#include "common.h"

/* Set up semaphores */
void init_semaphores(CircularQueue *image_queue, int buffer_size){
    /* Set up producer/consumer semaphores */
    sem_init(&image_queue->prod_sem, 1, 1);
    sem_init(&image_queue->cons_sem, 1, 1);

    /* Set up seq semaphores */
    sem_init(&image_queue->access_sem, 1, 1);
    /* Set up full/empty semaphores */
    sem_init(&image_queue->full, 1, 0);
    sem_init(&image_queue->empty, 1, buffer_size);
}

/* Set up the image queue */
void init_image_queue(CircularQueue *image_queue, int buffer_size){
    /* Set up semaphores */
    init_semaphores(image_queue, buffer_size);

    /* Allocate memory for uncomp_img */
    image_queue->uncomp_image = (char*)(image_queue + 1);
    image_queue->max_size = uncomp_size;
    image_queue->size = 0;

    /* Set up the array for buffers */
    image_queue->buf = (recv_buf**)(image_queue->uncomp_image + image_queue->max_size);

    /* Set up each entry of the array */
    for(int i = 0; i < buffer_size; i++){
        image_queue->buf[i] = (recv_buf *)((char*)(image_queue->buf + buffer_size) + i* (sizeof(recv_buf) + BUF_SIZE));
        image_queue->buf[i]->buf = (char *)(image_queue->buf[i] + 1);
        image_queue->buf[i]->size = 0;
        image_queue->buf[i]->max_size = BUF_SIZE;
        image_queue->buf[i]->seq = -1;
    }
    image_queue->capacity = buffer_size;
    image_queue->num = 0;

    /* Set up initial index for producer and consumer */
    image_queue->in = 0;
    image_queue->out = 0;
    /* Set up last sequence downloaded */
    image_queue->last_seq = 0;
    /* Set up counter for consumers */
    image_queue->counter = 0;
}