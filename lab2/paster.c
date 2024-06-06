#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <curl/curl.h>

#define BUF_SIZE 1048576  /* 1024*1024 = 1M */
#define BUF_INC  524288   /* 1024*512  = 0.5M */
#define ECE252_HEADER "X-Ece252-Fragment: "
#define MAX_THREADS 50    /* Maximum number of threads */

/* Create a global int to keep track of download of unique strips */
int count = 0;
/* Array to store the strips downloaded */
char *p_strips[MAX_THREADS];
/* Mutex for synchronizing access to p_strips and count */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

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
        size_t new_size = p->max_size + max(BUF_INC, realsize + 1);
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

    recv_buf.buf = malloc(BUF_SIZE);
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

        while(1){
            res = curl_easy_perform(curl_handle);
            if (res != CURLE_OK) {
                fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            }

            p_out->seq = recv_buf.seq;
            p_out->data = recv_buf.buf;
            p_out->size = recv_buf.size;
            /* If this strips has not been downloaded, download it */
            /* Lock the other threads */
            pthread_mutex_lock(&mutex);
            if (p_strips[(p_out->seq) - 1] == NULL) {
                p_strips[(p_out->seq) - 1] = p_out;
                count++;
                /* Check if all strips are downloaded */
                if (count == 50) {
                    pthread_mutex_unlock(&mutex);  // Unlock the mutex before exiting
                    break;  // Exit the while loop if all 50 strips are downloaded
                }
            } /* Else do nothing */
            pthread_mutex_unlock(&mutex);
        }

        /* After all strips have been downloaded, clean curl handle */
        curl_easy_cleanup(curl_handle);
    }

    curl_global_cleanup();

    return (void *)p_out;
}

void extract_and_write_chunk(FILE *output_file, char *data, size_t size, const char *chunk_type, int write_chunk_header) {
    size_t pos = 8; // skip PNG signature
    while (pos < size) {
        unsigned int length = ntohl(*(unsigned int *)(data + pos));
        char type[5];
        memcpy(type, data + pos + 4, 4);
        type[4] = 0;

        if (strcmp(type, chunk_type) == 0) {
            if (write_chunk_header) {
                fwrite(data + pos - 8, 1, length + 12, output_file);
            } else {
                fwrite(data + pos + 8, 1, length, output_file);
            }
        }
        pos += length + 12;
    }
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
                        fprintf(stderr, "Number of threads must be between 1 and %d\n", MAX_THREADS);
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

    pthread_t *p_tids = malloc(sizeof(pthread_t) * num_threads);
    struct thread_args *in_params = malloc(sizeof(struct thread_args) * num_threads);
    struct thread_ret **p_results = malloc(sizeof(struct thread_ret *) * num_threads);

    for (int i = 0; i < num_threads; i++) {
        snprintf(in_params[i].url, sizeof(in_params[i].url), "http://ece252-%d.uwaterloo.ca:2520/image?img=%d", (i % 3) + 1, img_num);
        in_params[i].thread_id = i;
        pthread_create(&p_tids[i], NULL, do_work, &in_params[i]);
    }

    FILE *output_file = fopen("all.png", "wb");
    if (!output_file) {
        perror("fopen");
        return 1;
    }

    // Write PNG signature
    unsigned char png_signature[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
    fwrite(png_signature, 1, sizeof(png_signature), output_file);

    for (int i = 0; i < num_threads; i++) {
        pthread_join(p_tids[i], (void **)&p_results[i]);

        if(p_strips[p_results[i]->seq - 1] != p_results[i]->data){
            free(p_results[i]->data);
            free(p_results[i]);
            p_results[i] = NULL;
        }
    }

    /* Write IHDR chunk */

    /* Write IDAT chunk */

    /* Write IEND chunk */

    fclose(output_file);

    free(p_tids);
    free(in_params);
    free(p_results);

    return 0;
}
