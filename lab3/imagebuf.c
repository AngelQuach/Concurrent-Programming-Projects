#include "imagebuf.h"

/* Buffer for storing image segments */
typedef struct{
    char *data;
    int seq;
    Imagebuf *next;
} Imagebuf;

typedef struct{
    Imagebuf *front;
    Imagebuf *rear;
    int capacity;
    int size;
}CircularQueue;

/* Create a node */
Imagebuf* create_imagebuf(const char *data, int seq){
    /* Initialize the node */
    Imagebuf *new_buf = (Imagebuf*)malloc(sizeof(Imagebuf));
    if(new_buf == NULL){
        perror("Failed to allocate memory for new imagebuf");
        return NULL;
    }
    new_buf->data = (char*)malloc(sizeof(data));
    if(new_buf->data == NULL){
        perror("Failed to allocate memory for image data");
        free(new_buf);
        return NULL;
    }
    memcpy(new_buf->data, data, sizeof(data));
    new_buf->seq = seq;
    new_buf->next = NULL;
    return new_buf;
}

/* Set up the image queue */
void init_image_queue(CircularQueue *image_queue, int buffer_size){
    /* Allocate memory for the queue */
    image_queue = (CircularQueue*)malloc(sizeof(CircularQueue));
    if(image_queue == NULL){
        perror("Failed to allocate memory for new queue");
        exit(EXIT_FAILURE);
    }
    image_queue->front = NULL;
    image_queue->rear = NULL;
    image_queue->capacity = buffer_size;
    image_queue->size = 0;
}