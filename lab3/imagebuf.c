#include "recv_buf.h"

/* Buffer for storing image segments */
typedef struct{
    char *buf;
    size_t size;
    size_t max_size;
    int seq;
    recv_buf *next;
} recv_buf;

/* Set up the Imagebuf */
recv_buf* init_image_buf(){
    /* Initialize the node */
    recv_buf *new_buf = (recv_buf*)malloc(sizeof(recv_buf));
    if(new_buf == NULL){
        perror("Failed to allocate memory for new imagebuf");
        return NULL;
    }
    new_buf->buf = NULL;
    new_buf->size = -1;
    new_buf->max_size = compressBound(uncomp_strip);
    new_buf->seq = -1;
    new_buf->next = NULL;
    return new_buf;
}