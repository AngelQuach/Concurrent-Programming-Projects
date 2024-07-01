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
#include <stdarg.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <semaphore.h>
#include "lab_png.h"
#include "crc.h"
#include "common.h"

#define BUF_SIZE 1048576  /* 1024*1024 = 1M */
#define BUF_INC  524288   /* 1024*512  = 0.5M */
#define ECE252_HEADER "X-Ece252-Fragment: "
#define MAX_STRIPS 50    /* Maximum number of threads */
#define const_width 400    /* Width of all png files */
#define final_height 300   /* Height of the final png file */
#define strip_height 6   /* Height of the individual strip */
#define uncomp_size 480300  /* total size of uncompressed strips */
#define strip_uncomp_size 9606  /* size of individual uncompressed strip */

int main(int argc, char **argv) {
    /* Set parameters based on input */
        if(argc != 6){
            fprintf(stderr, "Usage: %s <B> <P> <C> <X> <N>\n", argv[0]);
            return 0;
        }

    /* Read input */
        int B = (int)argv[1];           /* buf size */
        int P = (int)argv[2];           /* num of producers */
        int C = (int)argv[3];           /* num of consumers */
        int X = (int)argv[4];           /* milisec of consumer sleep */
        int N = (int)argv[5];           /* image num */

    /* Declare variables */
        /* Start and end time */
        double times[2];
        struct timeval tv;

    /* Declare shm */
        /* Create shared memory segment */
        CircularQueue *image_queue;
        int shm_size = sizeof(CircularQueue);
        int shm_id = shmget(IPC_PRIVATE, shm_size, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
        if(shm_id == -1) {
            perror("shmget");
            abort();
        }

        /* Attach shared memory segment */
        image_queue = (CircularQueue*)shmat(shm_id, NULL, 0);
        if(image_queue == (void *)-1){
            perror("shmat");
            exit(1);
        }
        /* Initialize circular queue in shared memory */  
        init_image_queue(&image_queue, B);

    /* Record starting time */
        if(gettimeofday(&tv, NULL) != 0){
            perror("gettimeofday");
            abort();
        }
        times[0] = (tv.tv_sec) + tv.tv_usec/1000000.;

    /* The following code is modified from starter code forkN.c */
        pid_t pid = 0;
        pid_t cpids[P + C];
        /* Create producer processes */
        for (int i = 0; i < P; i++) {
            pid = fork();

            if (pid < 0){        /* Error handling */
                perror("fork");
                abort();
            } 
        
            if (pid > 0) {        /* parent proc */
                cpids[i] = pid;
            } else{               /* child proc */
                producer_process(image_queue, (1 + (i%3)), N);
                exit(0);
            } 
        }

    /* Create consumer processes */
        for (int i = 0; i < C; i++) {
            pid = fork();

            if (pid < 0){        /* Error handling */
                perror("fork");
                abort();
            } 
        
            if (pid > 0) {        /* parent proc */
                cpids[P + i] = pid;
            } else{               /* child proc */
                /* Do consumer tasks */
                comsumer_process(image_queue, X );
                /* Cleanup needed */
                exit(0);
            }
        }
    
    /* parent process */
        if(pid > 0){
            /* Wait for all children to terminate */
            for(int i = P; i < (P + C); i++){
                waitpid(cpids[i], NULL, 0);
            }
            /* --- TO BE IMPLEMENTED --- */
            /* Compress image and build the file */
        }

    /* Cleanup all dynamic memory */
        /* Deattach shm */
        

    /* End time after all.png is generated */
        if(gettimeofday(&tv, NULL) != 0){
            perror("gettimeofday");
            abort();
        }
        times[1] = (tv.tv_sec) + tv.tv_usec/1000000.;
    
    /* Print the measured time */
        printf("%s execution time: %.6lf seconds\n", argv[0], times[1]-times[0]);

    return 0;
}
