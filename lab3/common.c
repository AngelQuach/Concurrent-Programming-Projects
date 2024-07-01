#include "common.h"

/* Set up semaphores */
void init_semaphores(CircularQueue *image_queue, int buffer_size){
    /* Set up producer/consumer semaphores */
    sem_init(&prod_sem, 0, 1);
    sem_init(&cons_sem, 0, 1);

    /* Set up seq semaphores */
    sem_init(&image_queue->access_sem, 0, 1);
    /* Set up full/empty semaphores */
    sem_init(&image_queue->full, 0, 0);
    sem_init(&image_queue->empty, 0, buffer_size);
}

/* Set up the image queue */
void init_image_queue(CircularQueue *image_queue, int buffer_size){
    /* Set up semaphores */
    init_semaphores(image_queue, buffer_size);

    /* Allocate memory for the queue */
    image_queue = (CircularQueue*)malloc(sizeof(CircularQueue));
    if(image_queue == NULL){
        perror("Failed to allocate memory for new queue");
        exit(EXIT_FAILURE);
    }

    /* Allocate memory for uncomp_img */
    image_queue->uncomp_image = (char*)malloc(sizeof(char)*uncomp_img);
    if(image_queue->uncomp_image == NULL){
        perror("Failed to allocate memory for uncomp_image");
        free(image_queue);
        exit(EXIT_FAILURE);
    }
    image_queue->max_size = uncomp_img;
    image_queue->size = 0;

    /* Set up the array for buffers */
    image_queue->buf = (recv_buf*)malloc(sizeof(recv_buf) * buffer_size);
    if(image_queue->buf == NULL){
        perror("Failed to allocate memory for image_queue buf array");
        free(image_queue->uncomp_image);
        free(image_queue);
        exit(EXIT_FAILURE);
    }
    image_queue->capacity = buffer_size;
    image_queue->num = 0;

    /* Set up initial index for producer and consumer */
    image_queue->in = 0;
    image_queue->out = 0;
}

/* Cleanup the image queue */
/* Note: semaphore cleanup are done by producers and consumers separately */
void cleanup_image_queue(CircularQueue *image_queue){
    if(image_queue == NULL){
        return;
    }

    /* Deallocate all recv_buf */
    while(image_queue->num < image_queue->capacity){
        recv_buf *temp = image_queue->buf[image_queue->num];
        if(temp != NULL){
            if(temp->buf != NULL){
                free(temp->buf);
            }
            free(temp);
            image_queue->buf[image_queue->num] = NULL;
        }
        image_queue->num++;
    }
    /* Free uncomp image */
    if(image_queue->uncomp_image != NULL){
        free(image_queue->uncomp_image);
    }
    free(image_queue);
}