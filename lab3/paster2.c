#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <curl/curl.h>
#include <stdarg.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>
#include "lab_png.h"
#include "crc.h"
#include "imagebuf.h"

#define BUF_SIZE 1048576  /* 1024*1024 = 1M */
#define BUF_INC  524288   /* 1024*512  = 0.5M */
#define ECE252_HEADER "X-Ece252-Fragment: "
#define MAX_STRIPS 50    /* Maximum number of threads */
#define const_width 400    /* Width of all png files */
#define final_height 300   /* Height of the final png file */
#define strip_height 6   /* Height of the individual strip */
#define uncomp_size 480300  /* total size of uncompressed strips */
#define strip_uncomp_size 9606  /* size of individual uncompressed strip */

/* Functions for constructing the final image */
int create_chunk(chunk_p *buf_chunk, const char *type_s, U32 width, U32 height, U8 *comp_png);
void write_chunk(FILE *fp, chunk_p buf_chunk);

/* The shm between producers and consumers */
CircularQueue *image_queue;

int main(int argc, char **argv) {
    /* Set parameters based on input */
    if(argc != 6){
        fprintf(stderr, "Usage: %s <B> <P> <C> <X> <N>\n", argv[0]);
        return 0;
    }

    /* Read input */
        /* buf size */
        int B = (int)argv[1];
        /* set up buffer */
        init_image_queue(image_queue, B);
        /* num of producers */
        int P = (int)argv[2];
        /* num of consumers */
        int C = (int)argv[3];
        /* milisec of consumer sleep */
        int X = (int)argv[4];
        /* image num */
        int N = (int)argv[5];
        /* Start and end time */
        double times[2];
        struct timeval tv;
        /* Set up url for connection */

    /* Record starting time */
    if(gettimeofday(&tv, NULL) != 0){
        perror("gettimeofday");
        abort();
    }
    time[0] = (tv.tv_sec) + tv.tv_usec/1000000.;

    /*  */
}