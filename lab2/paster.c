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
#include "lab_png.h"
#include "crc.h"

#define BUF_SIZE 1048576  /* 1024*1024 = 1M */
#define BUF_INC  524288   /* 1024*512  = 0.5M */
#define ECE252_HEADER "X-Ece252-Fragment: "
#define MAX_STRIPS 50    /* Maximum number of threads */
#define const_width 400    /* Width of all png files */
#define final_height 300   /* Height of the final png file */
#define strip_height 6   /* Height of the individual strip */
#define uncomp_size 480300  /* total size of uncompressed strips */
#define strip_uncomp_size 9606  /* size of individual uncompressed strip */

/* Create a global int to keep track of download of unique strips */
int count = 0;
/* Array to store the strips downloaded */
struct thread_ret *p_strips[MAX_STRIPS];
/* Mutex for synchronizing access to p_strips and count */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

#define max(a, b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

struct thread_args {
    int thread_id;
    char url[256];
};

struct thread_ret {
    int seq;
    char *data;
    size_t size;
};

struct recv_buf {
    char *buf;
    size_t size;
    size_t max_size;
    int seq;
};

size_t header_cb_curl(char *p_recv, size_t size, size_t nmemb, void *userdata) {
    int realsize = size * nmemb;
    struct recv_buf *p = userdata;
    
    if (realsize > strlen(ECE252_HEADER) &&
        strncmp(p_recv, ECE252_HEADER, strlen(ECE252_HEADER)) == 0) {
        p->seq = atoi(p_recv + strlen(ECE252_HEADER));
    }
    return realsize;
}

size_t write_cb_curl(char *p_recv, size_t size, size_t nmemb, void *p_userdata) {
    size_t realsize = size * nmemb;
    struct recv_buf *p = (struct recv_buf *)p_userdata;
    
    if (p->size + realsize + 1 > p->max_size) {
        size_t new_size = p->max_size + max((size_t)BUF_INC, realsize + 1);
        char *q = realloc(p->buf, new_size);
        if (q == NULL) {
            perror("realloc");
            return -1;
        }
        p->buf = q;
        p->max_size = new_size;
    }
    memcpy(p->buf + p->size, p_recv, realsize);
    p->size += realsize;
    p->buf[p->size] = 0;

    return realsize;
}

void *do_work(void *arg) {
    struct thread_args *p_in = arg;
    struct thread_ret *p_out = malloc(sizeof(struct thread_ret));
    struct recv_buf recv_buf;

    if (p_out == NULL) {
        fprintf(stderr, "Failed to allocate memory for thread_ret\n");
        return NULL;
    }

    recv_buf.buf = malloc(BUF_SIZE);

    if (recv_buf.buf == NULL) {
        fprintf(stderr, "Failed to allocate memory for recv_buf\n");
        free(p_out);
        return NULL;
    }

    recv_buf.size = 0;
    recv_buf.max_size = BUF_SIZE;
    recv_buf.seq = -1;

    CURL *curl_handle;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl_handle = curl_easy_init();
    if (curl_handle) {
        curl_easy_setopt(curl_handle, CURLOPT_URL, p_in->url);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_cb_curl);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &recv_buf);
        curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_cb_curl);
        curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, &recv_buf);
        curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

        while(count < MAX_STRIPS){
            res = curl_easy_perform(curl_handle);
            if (res != CURLE_OK) {
                fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            }

            p_out->seq = recv_buf.seq;
            p_out->data = recv_buf.buf;
            p_out->size = recv_buf.size;
            /* If this strips has not been downloaded, download it */
            if (p_out->seq >= 0 && p_out->seq < MAX_STRIPS) {
                /* Lock the other threads */
                pthread_mutex_lock(&mutex);

                if(p_strips[(p_out->seq)] == NULL){
                    /* TEST: print Thread # and sequence number */
                    printf("Thread %d: downloading strip %d\n", p_in->thread_id, p_out->seq);


                    p_strips[(p_out->seq)] = p_out;
                    count++;
                    /* Check if all strips are downloaded */
                    if (count >= 50) {
                        printf("Thread %d: All strips downloaded, exiting...\n", p_in->thread_id);
                        /* Unlock the mutex before exiting */
                        pthread_mutex_unlock(&mutex);
                        /* Exit the while loop if all 50 strips are downloaded */
                        break;
                    }
                }
                pthread_mutex_unlock(&mutex);
            }
        }

        /* After all strips have been downloaded, clean curl handle */
        curl_easy_cleanup(curl_handle);
        curl_global_cleanup();

        return (void *)p_out;
    }
    /* In case curl_easy_init() failed */
    curl_global_cleanup();
    free(p_out);
    free(recv_buf.buf);
    return NULL;
}

/* initiate and set up the chunk of type_s */
int create_chunk(chunk_p *buf_chunk, const char *type_s, U32 width, U32 height, U8 *comp_png){
    /* pointer to chunk created */
    (*buf_chunk) = malloc(sizeof(struct chunk));
    if((*buf_chunk) == NULL) return -1;
    
    /* determine which chunk we are creating */
    if(!strcmp(type_s, "IHDR")){
        /* IHDR length and type */
            (*buf_chunk)->length = DATA_IHDR_SIZE;
            memcpy((*buf_chunk)->type, type_s, 4);

        /* IHDR data */
            data_IHDR_p IHDR_d = malloc(DATA_IHDR_SIZE);
            if(IHDR_d == NULL){
                free((*buf_chunk));
                return -1;
            }
            memset(IHDR_d, 0, DATA_IHDR_SIZE);

            IHDR_d->width = htonl(width);
            IHDR_d->height = htonl(height);
            IHDR_d->bit_depth = 8;
            IHDR_d->color_type = 6;
            IHDR_d->compression = 0;
            IHDR_d->filter = 0;
            IHDR_d->interlace = 0;
            (*buf_chunk)->p_data = (void *) IHDR_d;
    } else if(!strcmp(type_s, "IDAT")){
        /* IDAT length and type */
            (*buf_chunk)->length = width;
            memcpy((*buf_chunk)->type, type_s, 4);

        /* IDAT data */
            (*buf_chunk)->p_data = comp_png;
    } else if(!strcmp(type_s, "IEND")){
        /* IEND length and type */
            (*buf_chunk)->length = 0;
            memcpy((*buf_chunk)->type, type_s, 4);
        
        /* IEND data */
            (*buf_chunk)->p_data = NULL;
    } else{
        /* Unsupported chunk type */
        free((*buf_chunk));
        return -1;
    }

    /* Set up chunk crc */
        U8 *temp_buf = malloc(CHUNK_TYPE_SIZE + (*buf_chunk)->length);
        if(temp_buf == NULL){
            if(!strcmp(type_s, "IHDR")){
                free((*buf_chunk)->p_data);
            }
            free((*buf_chunk));
            return -1;
        }
        memcpy(temp_buf, (*buf_chunk)->type, CHUNK_TYPE_SIZE);
        /* for IHDR/IDAT chunks */
        if((*buf_chunk)->length > 0 && (*buf_chunk)->p_data != NULL){
            memcpy(temp_buf + CHUNK_TYPE_SIZE, (*buf_chunk)->p_data, (*buf_chunk)->length);
        }
        (*buf_chunk)->crc = crc(temp_buf, CHUNK_TYPE_SIZE + (*buf_chunk)->length);
        free(temp_buf);
    
    /* chunk set up completed */
        return 0;
}

/* write buf_chunk to the file fp */
void write_chunk(FILE *fp, chunk_p buf_chunk){
    /* Write length and type */
    U32 chunk_len = htonl(buf_chunk->length);
    fwrite(&chunk_len, 1, CHUNK_LEN_SIZE, fp);
    fwrite(buf_chunk->type, 1, CHUNK_TYPE_SIZE, fp);

    /* Check if data exists, write data */
    if(buf_chunk->p_data != NULL){
        fwrite(buf_chunk->p_data, 1, buf_chunk->length, fp);
    }

    /* Write crc */
    U32 crc_value = htonl(buf_chunk->crc);
    fwrite(&crc_value, 1, CHUNK_CRC_SIZE, fp);
}

int main(int argc, char **argv) {
    int num_threads = 1;
    int img_num = 1;

    int opt;
    while ((opt = getopt(argc, argv, "t:n:")) != -1) {
        switch (opt) {
            case 't':
                if(optarg != NULL && *optarg != '\0'){
                    num_threads = strtoul(optarg, NULL, 10);
                    if(num_threads <= 0){
                        fprintf(stderr, "Number of threads must be at least 1\n");
                        return 1;
                    }
                }
                break;
            case 'n':
                if(optarg != NULL && *optarg != '\0'){
                    img_num = strtoul(optarg, NULL, 10);
                    if(img_num <= 0){
                        fprintf(stderr, "Image number must be 1, 2, or 3\n");
                        return 1;
                    }
                }
                break;
            default:
                return 1;
        }
    }

    /* Initialize p_strips to NULL */
    for (int i = 0; i < MAX_STRIPS; i++) {
        p_strips[i] = NULL;
    }


    pthread_t *p_tids = malloc(sizeof(pthread_t) * num_threads);
    struct thread_args *in_params = malloc(sizeof(struct thread_args) * num_threads);
    struct thread_ret **p_results = malloc(sizeof(struct thread_ret *) * num_threads);

    if (p_tids == NULL || in_params == NULL || p_results == NULL) {
        fprintf(stderr, "Failed to allocate memory for thread arrays\n");
        if(p_tids != NULL){
            free(p_tids);
        }
        if(in_params != NULL){
            free(in_params);
        }
        if(p_results != NULL){
            free(p_results);
        }
        return 1;
    }

    for (int i = 0; i < num_threads; i++) {
        snprintf(in_params[i].url, sizeof(in_params[i].url), "http://ece252-%d.uwaterloo.ca:2520/image?img=%d", (i % 3) + 1, img_num);
        in_params[i].thread_id = i;
        pthread_create(&p_tids[i], NULL, do_work, &in_params[i]);
    }

    FILE *output_file = fopen("all.png", "wb");
    if (!output_file) {
        perror("fopen");
        free(p_tids);
        free(in_params);
        free(p_results);

        for (int i = 0; i < MAX_STRIPS; i++) {
            if(p_strips[i] != NULL){
                if(p_strips[i]->data != NULL){
                    free(p_strips[i]->data);
                    p_strips[i] = NULL;
                }
                free(p_strips[i]);
                p_strips[i] = NULL;
            }
        }

        for(int i = 0; i < num_threads; i++){
            if(p_results[i] != NULL){
                if(p_results[i]->data != NULL){
                    free(p_results[i]->data);
                    p_results[i]->data = NULL;
                }
                free(p_results[i]);
                p_results[i] = NULL;
            }
        }

        return 1;
    }

    // Write PNG signature
    unsigned char png_signature[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
    fwrite(png_signature, 1, sizeof(png_signature), output_file);

    for (int i = 0; i < num_threads; i++) {
        pthread_join(p_tids[i], (void **)&p_results[i]);
        if (p_results[i] != NULL) {
            printf("Thread %d: Finished with seq %d\n", i, p_results[i]->seq);  // Debug info
        } else {
            printf("Thread %d: Finished with no result\n", i);  // Debug info
        }
    }

    printf("Download complete!\n");

    /* Write IHDR chunk */
        /* Set up IHDR chunk */
            chunk_p p_IHDR;
            if(create_chunk(&p_IHDR, "IHDR", const_width, final_height, NULL) != 0){
                printf("Error allocating memory for IHDR chunk for new file\n");
                fclose(output_file);
                free(p_tids);
                free(in_params);
                free(p_results);

                for (int i = 0; i < MAX_STRIPS; i++) {
                    if(p_strips[i] != NULL){
                        if(p_strips[i]->data != NULL){
                            free(p_strips[i]->data);
                            p_strips[i] = NULL;
                        }
                        free(p_strips[i]);
                        p_strips[i] = NULL;
                    }
                }

                for(int i = 0; i < num_threads; i++){
                    if(p_results[i] != NULL){
                        if(p_results[i]->data != NULL){
                            free(p_results[i]->data);
                            p_results[i]->data = NULL;
                        }
                        free(p_results[i]);
                        p_results[i] = NULL;
                    }
                }

                return -1;
            }
        /* IHDR completed */
        write_chunk(output_file, p_IHDR);

    printf("IHDR chunk write complete!\n");

    /* Write IDAT chunk */
        /* A buffer to stored the concatenated uncompressed strips */
            U8 *total_uncomp_strip = (U8*)malloc(uncomp_size);
            if(total_uncomp_strip == NULL){
                printf("Error allocating memory for total_uncomp_strip\n");
                fclose(output_file);
                free(p_tids);
                free(in_params);
                free(p_results);
                free(p_IHDR->p_data);
                free(p_IHDR);

                for (int i = 0; i < MAX_STRIPS; i++) {
                    if(p_strips[i] != NULL){
                        if(p_strips[i]->data != NULL){
                            free(p_strips[i]->data);
                            p_strips[i] = NULL;
                        }
                        free(p_strips[i]);
                        p_strips[i] = NULL;
                    }
                }

                for(int i = 0; i < num_threads; i++){
                    if(p_results[i] != NULL){
                        if(p_results[i]->data != NULL){
                            free(p_results[i]->data);
                            p_results[i]->data = NULL;
                        }
                        free(p_results[i]);
                        p_results[i] = NULL;
                    }
                }

                return -1;
            }

        /* A buffer to store individual compressed IDAT data */
            U8 *p_comp_IDAT;
        /* A buffer to store individual uncompressed IDAT data */
            U8 *buf_strip_uncomp;

        /* Concatenate uncompressed strips */
        for(int i = 0; i < MAX_STRIPS; i++){
            if(p_strips[i] != NULL){
                /* length of compressed strip IDAT data */
                int strip_length;
                memcpy(&strip_length, p_strips[i]->data + 25 + 8, 4);
                strip_length = ntohl(strip_length);
                p_comp_IDAT = (U8*)malloc(sizeof(char)*strip_length);
                if(p_comp_IDAT == NULL){
                    printf("Error allocating memory for p_comp_IDAT\n");
                    fclose(output_file);
                    free(p_tids);
                    free(in_params);
                    free(p_results);
                    free(p_IHDR->p_data);
                    free(p_IHDR);
                    free(total_uncomp_strip);
                    free(buf_strip_uncomp);

                    for (int i = 0; i < MAX_STRIPS; i++) {
                        if(p_strips[i] != NULL){
                            if(p_strips[i]->data != NULL){
                                free(p_strips[i]->data);
                                p_strips[i] = NULL;
                            }
                            free(p_strips[i]);
                            p_strips[i] = NULL;
                        }
                    }

                    for(int i = 0; i < num_threads; i++){
                        if(p_results[i] != NULL){
                            if(p_results[i]->data != NULL){
                                free(p_results[i]->data);
                                p_results[i]->data = NULL;
                            }
                            free(p_results[i]);
                            p_results[i] = NULL;
                        }
                    }

                    return -1;
                }
                
                U64 copy_strip_uncomp_size = (U64)strip_uncomp_size;
                U64 copy_strip_length = (U64)strip_length;
                buf_strip_uncomp = malloc(sizeof(char)*copy_strip_uncomp_size);
                if(buf_strip_uncomp == NULL){
                    printf("Unable to allocate memory for uncompressed stip buffer\n");
                    fclose(output_file);
                    free(p_tids);
                    free(in_params);
                    free(p_results);
                    free(p_IHDR->p_data);
                    free(p_IHDR);
                    free(total_uncomp_strip);
                    free(p_comp_IDAT);

                    for (int i = 0; i < MAX_STRIPS; i++) {
                        if(p_strips[i] != NULL){
                            if(p_strips[i]->data != NULL){
                                free(p_strips[i]->data);
                                p_strips[i] = NULL;
                            }
                            free(p_strips[i]);
                            p_strips[i] = NULL;
                        }
                    }

                    for(int i = 0; i < num_threads; i++){
                        if(p_results[i] != NULL){
                            if(p_results[i]->data != NULL){
                                free(p_results[i]->data);
                                p_results[i]->data = NULL;
                            }
                            free(p_results[i]);
                            p_results[i] = NULL;
                        }
                    }
                    return -1;
                }

                /* Read IDAT data to pointer */
                memcpy(p_comp_IDAT, p_strips[i]->data + 25 + 8 + 8, strip_length);

                /* Decompress IDAT data and save to buffer */
                if(mem_inf(buf_strip_uncomp, &copy_strip_uncomp_size, p_comp_IDAT, copy_strip_length) != Z_OK) {
                    printf("Decompression failed for strip %d\n", i);
                    continue;
                }
                memcpy(total_uncomp_strip + i*copy_strip_uncomp_size, buf_strip_uncomp, copy_strip_uncomp_size); 
                free(p_comp_IDAT);
                free(buf_strip_uncomp);  
            } else{
                printf("Cannot access strip %d\n", i);
                continue;
            }
        }

        /* Compress the buffer and stored in buffer */
        U8 *total_comp = (U8*)malloc(sizeof(char)*compressBound(uncomp_size));
        if(total_comp == NULL){
            printf("Error allocating memory for total_comp\n");
            fclose(output_file);
            free(p_tids);
            free(in_params);
            free(p_results);
            free(p_IHDR->p_data);
            free(p_IHDR);
            free(total_uncomp_strip);
            free(p_comp_IDAT);

            for (int i = 0; i < MAX_STRIPS; i++) {
                if(p_strips[i] != NULL){
                    if(p_strips[i]->data != NULL){
                        free(p_strips[i]->data);
                        p_strips[i] = NULL;
                    }
                    free(p_strips[i]);
                    p_strips[i] = NULL;
                }
            }

            for(int i = 0; i < num_threads; i++){
                if(p_results[i] != NULL){
                    if(p_results[i]->data != NULL){
                        free(p_results[i]->data);
                        p_results[i]->data = NULL;
                    }
                    free(p_results[i]);
                    p_results[i] = NULL;
                }
            }

            return -1;
        }
        U64 copy_comp_size = (U64)compressBound(uncomp_size);
        U64 copy_uncomp_size = (U64)uncomp_size;
        if(mem_def(total_comp, &copy_comp_size, total_uncomp_strip, copy_uncomp_size, Z_DEFAULT_COMPRESSION) != Z_OK){
            printf("Error compressing png files\n");
        }

        /* Create the IDAT chunk and write to file */
            chunk_p p_IDAT;
            if(create_chunk(&p_IDAT, "IDAT", (U32)copy_comp_size, 0, total_comp) != 0){
                printf("Error allocating memory for IDAT chunk for new file\n");
                fclose(output_file);
                free(p_tids);
                free(in_params);
                free(p_results);
                free(p_IHDR->p_data);
                free(p_IHDR);
                free(total_uncomp_strip);
                free(p_comp_IDAT);

                for (int i = 0; i < MAX_STRIPS; i++) {
                    if(p_strips[i] != NULL){
                        if(p_strips[i]->data != NULL){
                            free(p_strips[i]->data);
                            p_strips[i] = NULL;
                        }
                        free(p_strips[i]);
                        p_strips[i] = NULL;
                    }
                }

                for(int i = 0; i < num_threads; i++){
                    if(p_results[i] != NULL){
                        if(p_results[i]->data != NULL){
                            free(p_results[i]->data);
                            p_results[i]->data = NULL;
                        }
                        free(p_results[i]);
                        p_results[i] = NULL;
                    }
                }

                return -1;
            }
        /* IDAT completed */
        write_chunk(output_file, p_IDAT);

        printf("IDAT chunk write complete!\n");

    /* Write IEND chunk */
        /* Set up IEND chunk */
            chunk_p p_IEND;
            if(create_chunk(&p_IEND, "IEND", 0, 0, NULL) != 0){
                printf("Error allocating memory for IEND chunk for new file\n");
                fclose(output_file);
                free(p_tids);
                free(in_params);
                free(p_results);
                free(p_IHDR->p_data);
                free(p_IHDR);
                free(total_uncomp_strip);
                free(p_comp_IDAT);
                free(p_IDAT);

                for (int i = 0; i < MAX_STRIPS; i++) {
                    if(p_strips[i] != NULL){
                        if(p_strips[i]->data != NULL){
                            free(p_strips[i]->data);
                            p_strips[i] = NULL;
                        }
                        free(p_strips[i]);
                        p_strips[i] = NULL;
                    }
                }

                for(int i = 0; i < num_threads; i++){
                    if(p_results[i] != NULL){
                        if(p_results[i]->data != NULL){
                            free(p_results[i]->data);
                            p_results[i]->data = NULL;
                        }
                        free(p_results[i]);
                        p_results[i] = NULL;
                    }
                }

                return -1;
            }
        /* IEND completed */
        write_chunk(output_file, p_IEND);

        printf("IEND chunk write complete!\n");

    fclose(output_file);

    for (int i = 0; i < MAX_STRIPS; i++) {
        if(p_strips[i] != NULL){
            if(p_strips[i]->data != NULL){
                p_strips[i] = NULL;
            }
            free(p_strips[i]);
            p_strips[i] = NULL;
        }
    }

    for(int i = 0; i < num_threads; i++){
        if(p_results[i] != NULL){
            if(p_results[i]->data != NULL){
                free(p_results[i]->data);
                p_results[i]->data = NULL;
            }
            free(p_results[i]);
            p_results[i] = NULL;
        }
    }

    free(p_tids);
    free(in_params);
    free(p_results);
    free(total_uncomp_strip);
    free(p_IHDR->p_data);
    free(p_IHDR);
    free(total_comp);
    free(p_IDAT);
    free(p_IEND);

    return 0;
}