#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "common.h"
#include "recv_buf.h"



recv_buf *segment = NULL;
int counter = 0;


void *comsumer(void *shm, void *arg){
    CircularQueue *queue = (CircularQueue *)shm;
     int X = *((int *)arg); // Consumer sleep time in milliseconds
     while (1) {
        sem_wait(&queue->full);
        sem_wait(&cons_sem);

        if(queue->num > 0){
            /*retrieve the item from the buffer at the position queue->out*/
            segment = queue->buf[queue->out];
            /*updates the index to point to the next position of the circular queue*/
            queue->out = (queue->out +1) % queue->capacity;
            /*reflected that one item has been consumed*/
            queue->num --;
            counter ++;


            /*Sleep for a specified duration and simulate processing time*/
            usleep(X* 1000);

            /* Read IDAT data to pointer*/
            unsigned char *p_comp_IDAT = (unsigned char *)malloc(segment->size);
            memcpy(p_comp_IDAT, segment->data + 25 + 8 + 8, segment->size);

            /* uncompressed size of individual strip */
            size_t copy_strip_uncomp_size = 9606;
            /* Allocate memory for decompressed data */
            char *buf_strip_uncomp = malloc(sizeof(char)*copy_strip_uncomp_size);
    
            /*Decompress IDAT data and save to buffer*/
            if(meminf(buf_strip_uncomp, &copy_strip_uncomp_size, p_comp_IDAT, segment->size)){
                    printf("Decompression failed for segment %d", segment->seq);
                    free(p_comp_IDAT);
                    free(buf_strip_uncomp);
                    continue;
            }

            /*Store the decompressed segment into the uncom_image buffer*/
            size_t offset = segment->seq * copy_strip_uncomp_size;
            memcpy(queue->uncomp_image + offset, buf_strip_uncomp, copy_strip_uncomp_size );

            sem_post(&cons_sem);
            sem_post(&queue->empty); /*signal that there's an empty slot*/

            /*free the allocated memory*/
            free(p_comp_IDAT);
            free(buf_strip_uncomp);
            free(segment->data);

            if(counter == 50){
                break;
            }
        }else{
            sem_post(&cons_sem);
            sem_post(&queue->full);
        }
     }
}

