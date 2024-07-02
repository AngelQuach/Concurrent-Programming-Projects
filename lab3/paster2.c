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
#include "zutil.h"
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
        int B = atoi(argv[1]);           /* buf size */
        int P = atoi(argv[2]);           /* num of producers */
        int C = atoi(argv[3]);           /* num of consumers */
        int X = atoi(argv[4]);           /* milisec of consumer sleep */
        int N = atoi(argv[5]);           /* image num */

    /* Declare variables */
        /* Start and end time */
        double times[2];
        struct timeval tv;

    /* Declare shm */
        /* Create shared memory segment */
        CircularQueue *image_queue;
        int shm_size = sizeof(CircularQueue) + B * (sizeof(recv_buf *) + sizeof(recv_buf) + BUF_SIZE) + uncomp_size;
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
        init_image_queue(image_queue, B);

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
        for (int i = 0; i < (P + C); i++) {
            pid = fork();

            if (pid < 0){        /* Error handling */
                perror("fork");
                abort();
            } 
        
            if (pid == 0){               /* child proc */
                if(i < P){            /* Let child be producer if i is even */
                    /* Do producer tasks */
                    producer_process(image_queue, (1 + (i%3)), N);
                    exit(0);
                } else{                     /* Let child be consumer if i is odd */
                    /* Do consumer tasks */
                    comsumer_process(image_queue, X);
                    exit(0);
                }
            } 
            cpids[i] = pid;
        }
    
    /* parent process */
        if(pid > 0){
            /* Wait for all children to terminate */
            for(int i = 0; i < (P + C); i++){
                waitpid(cpids[i], NULL, 0);
            }

            /* Compress image and build the file */
            U8 *total_comp = (U8*)malloc(sizeof(char)*compressBound(uncomp_size));
            if(total_comp == NULL){
                perror("Error allocating memory for total_comp");
                abort();
            }
            U64 copy_comp_size = (U64)compressBound(uncomp_size);
            U64 copy_uncomp_size = (U64)uncomp_size;
            if(mem_def(total_comp, &copy_comp_size, (U8*)image_queue->uncomp_image, copy_uncomp_size, Z_DEFAULT_COMPRESSION) != Z_OK){
                printf("Error compressing png files\n");
            }

            /* Write the image into the file */
                FILE *output_file = fopen("all.png", "wb");
                if (!output_file) {
                    perror("fopen");
                    abort();
                }

                /* Write signature */
                unsigned char png_signature[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
                fwrite(png_signature, 1, sizeof(png_signature), output_file);

                /* Write IHDR chunk */
                    /* Set up IHDR chunk */
                    chunk_p p_IHDR;
                    if(create_chunk(&p_IHDR, "IHDR", const_width, final_height, NULL) != 0){
                        perror("Error allocating memory for IHDR chunk for new file");
                        fclose(output_file);
                        abort();
                    }
                    /* IHDR completed */
                    write_chunk(output_file, p_IHDR);
                    free(p_IHDR->p_data);
                    free(p_IHDR);

                /* Write IDAT chunk */
                    /* Set up IDAT chunk */
                    chunk_p p_IDAT;
                    if(create_chunk(&p_IDAT, "IDAT", (U32)copy_comp_size, 0, total_comp) != 0){
                        perror("Error allocating memory for IDAT chunk for new file");
                        fclose(output_file);
                        abort();
                    }
                    /* IDAT completed */
                    write_chunk(output_file, p_IDAT);
                    free(p_IDAT->p_data);
                    free(p_IDAT);

                /* Write IEND chunk */
                    /* Set up IEND chunk */
                    chunk_p p_IEND;
                    if(create_chunk(&p_IEND, "IEND", 0, 0, NULL) != 0){
                        perror("Error allocating memory for IEND chunk for new file");
                        fclose(output_file);
                        abort();
                    }
                    /* IEND completed */
                    write_chunk(output_file, p_IEND);
                    free(p_IEND);

            /* File is finished */
            fclose(output_file);
        }

    /* End time after all.png is generated */
        if(gettimeofday(&tv, NULL) != 0){
            perror("gettimeofday");
            abort();
        }
        times[1] = (tv.tv_sec) + tv.tv_usec/1000000.;
    
    /* Print the measured time */
        printf("%s execution time: %.6lf seconds\n", argv[0], times[1]-times[0]);

    /* Cleanup all dynamic memory */
        /* Detach shm */
        if(shmdt(image_queue) == -1){
            perror("shmdt");
            exit(1);
        }
        /* Remove shm */
        if(shmctl(shm_id, IPC_RMID, NULL) == -1){
            perror("shmctl");
            exit(1);
        }

    return 0;
}