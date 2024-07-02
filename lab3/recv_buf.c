#include "recv_buf.h"
#include <zlib.h>

#define BUF_SIZE 1048576  /* 1024*1024 = 1M */

/* Set up the Imagebuf */
recv_buf* init_image_buf(){
    /* Initialize the node */
    recv_buf *new_buf = (recv_buf*)malloc(sizeof(recv_buf));
    if(new_buf == NULL){
        perror("Failed to allocate memory for new imagebuf");
        return NULL;
    }
    new_buf->buf = malloc(BUF_SIZE);
    if(new_buf->buf == NULL){
        perror("Failed to allocate memory for new_buf->buf");
        free(new_buf);
        return NULL;
    }
    new_buf->size = 0;
    new_buf->max_size = BUF_SIZE;
    new_buf->seq = -1;
    return new_buf;
}