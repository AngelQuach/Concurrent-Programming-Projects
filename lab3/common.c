#include "common.h"

/* Set up the image queue */
void init_image_queue(CircularQueue *image_queue, int buffer_size){
    /* Allocate memory for the queue */
    image_queue = (CircularQueue*)malloc(sizeof(CircularQueue));
    if(image_queue == NULL){
        perror("Failed to allocate memory for new queue");
        exit(EXIT_FAILURE);
    }
    /* Allocate memory for uncomp_img */
    image_queue->uncomp_image = (char*)malloc(sizeof(char*)*uncomp_img);
    image_queue->max_size = uncomp_img;
    image_queue->size = 0;
    /* Set up the array for buffers */
    image_queue->buf = (recv_buf*)malloc(buffer_size*sizeof(recv_buf));
    image_queue->capacity = buffer_size;
    image_queue->num = 0;
}

/* Cleanup the image queue */
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
    return;
}