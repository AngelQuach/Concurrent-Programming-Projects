#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "data_structure.h"     /* For data strucutres */
#include "thread_func.h"        /* For pthreads */
#include "main.h"               /* For CURL functions */

#define MAX_URLS 500
#define TABLE_SIZE 500
#define MAX_PNG_URLS 500
#define URL_SIZE 256
#define BUF_SIZE 1048576  /* 1024*1024 = 1M */

CircularQueue url_frontiers;          /* url_Frontiers array for storing all urls */
HashTable url_visited;                /* url_visited array for storing visited urls */
URL_Array PNG_url;                    /* PNG_url array for storing identified PNG urls */

/* Define default variables */
int num_threads = 1;
int num_unique_PNG = 50;
int if_logfile = 0;
char *log_filename = NULL;
char *input_url = NULL;

/* Declare pthread function here */
void *handle_url();

int main(int argc, char* argv[]) 
{
    /* Get input from the user */
    int opt;
    while ((opt = getopt(argc, argv, "t:m:v:")) != -1) {
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
            case 'm':
                if(optarg != NULL && *optarg != '\0'){
                    num_unique_PNG = strtoul(optarg, NULL, 10);
                    if(num_unique_PNG <= 0){
                        fprintf(stderr, "Image number must be 1, 2, or 3\n");
                        return 1;
                    }
                }
                break;
            case 'v':
                if(optarg != NULL && *optarg != '\0'){
                    if_logfile = 1;
                    log_filename = optarg;
                }
                break;
            default:
                return 1;
        }
    }

    /* Stored the base_url to be accessed */
    if (optind < argc) {
        input_url = strdup(argv[optind]);
    } else {
        fprintf(stderr, "URL is required\n");
        return 1;
    }

    /* Set up shared url_frontier */
        initQueue(&url_frontiers); 
        /* Add SEED url to the CircularQueue */
        while(enqueue(&url_frontiers, input_url) != 0);

    /*Set up url_visited*/
        initHashTable(&url_visited);

    /*Set up PNG_url*/
        initArray(&PNG_url);

    /* Declare variables */
        /* Start and end time */
        double times[2];
        struct timeval tv;

    /* Record starting time */
        if(gettimeofday(&tv, NULL) != 0){
            perror("gettimeofday");
            abort();
        }
        times[0] = (tv.tv_sec) + tv.tv_usec/1000000.;

    /* Var. for threads */
    pthread_t *p_tids = malloc(sizeof(pthread_t) * num_threads);
        if (p_tids == NULL) {
            fprintf(stderr, "Failed to allocate memory for thread arrays\n");
            return 1;
        }
    /* Call the url_handling function for each thread */
        for(int i = 0; i < num_threads; i++){
            pthread_create(&p_tids[i], NULL, handle_url, NULL);
        }
    /* Wait for all threads to complete */
        for (int i = 0; i < num_threads; i++) {
            pthread_join(p_tids[i], NULL);
        }

    /* Create png urls file */
        FILE *f_output = fopen("png_urls.txt", "w");
        if (!f_output) {
            perror("fopen");
            abort();
        }
    /* Now print to ouput file */
        for(int i = 0; i < PNG_url.count; i++){
            if (PNG_url.urls[i] != NULL) {
                fprintf(f_output, "%s\n", PNG_url.urls[i]);
            }
        }
    /* Close the file */
        fclose(f_output);

    /* Check if we need to create the logfile*/
        if(if_logfile == 1){
            /* Create png urls file */
            f_output = fopen(log_filename, "w");
            if (!f_output) {
                perror("fopen");
                abort();
            }
        /* Now print to ouput file */
            for(int i = 0; i < TABLE_SIZE; i++){
                if (url_visited.buckets[i] != NULL) {
                    fprintf(f_output, "%s\n", url_visited.buckets[i]->url);
                }
            }
        /* Close the file */
            fclose(f_output);
        }

     /* End time after all.png is generated */
        if(gettimeofday(&tv, NULL) != 0){
            perror("gettimeofday");
            abort();
        }
        times[1] = (tv.tv_sec) + tv.tv_usec/1000000.;
    
    /* Print the measured time */
        printf("%s execution time: %.6lf seconds\n", argv[0], times[1]-times[0]);    

    /* Cleanup all data structures */
    free(input_url);
    free(p_tids);
    freeQueue(&url_frontiers);
    freeHashTable(&url_visited);
    freeArray(&PNG_url);
    xmlCleanupParser();   
}

/* Function for thread to handle url */
void *handle_url(){
    /* Initialize connection */
    curl_global_init(CURL_GLOBAL_DEFAULT);

    /* Check if we have enough unique PNG files yet */
    while(countPNGURL(&PNG_url) < num_unique_PNG){       /* While we haven't reached the desired PNG url count */
        /* recv_buf to stored output from request */
        RECV_BUF *recv_buf = malloc(sizeof(RECV_BUF));
        if(recv_buf == NULL){
            printf("failed to allocate memory for recv_buf\n");
            continue;
        }
        
        /* Check if url has been visited */
            /* Copy the url from array*/
            char *url = dequeue(&url_frontiers, &num_threads);
            if(url == NULL){
                /* Deallocate dynamic memory */
                free(recv_buf);
                break;
            }

            if(checkHashURL(&url_visited, url) == 1){          /* If visited, move on to next url in list */
                /* Deallocate dynamic memory */
                free(recv_buf);
                free(url);
                continue;
            }

        /* If not visited add to url visited array */
            addHashURL(&url_visited, url);

        /* Send a request to url */
            /* Initialize connection */
            CURL *curl_handle = easy_handle_init(recv_buf, url);
            if (curl_handle == NULL) {
                fprintf(stderr, "Curl initialization failed. Exiting...\n");
                curl_easy_cleanup(curl_handle);
                recv_buf_cleanup(recv_buf);
                free(recv_buf);
                free(url);
                curl_global_cleanup();
                abort();
            }
            CURLcode res = curl_easy_perform(curl_handle);
            
        /* If unexpected response */
            if(res != CURLE_OK) {    
                //fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
                curl_easy_cleanup(curl_handle);
                recv_buf_cleanup(recv_buf);
                free(recv_buf);
                free(url);
                continue;                  /* skip the current url */
            }
            
        /* Otherwise check response code by calling general process function*/
            process_data(&url_frontiers, &PNG_url, curl_handle, recv_buf);
        
        /* Reset curl_handle and recv_buf before next loop starts */
            curl_easy_cleanup(curl_handle);
            recv_buf_cleanup(recv_buf);
            free(recv_buf);
            free(url);
    }
    curl_global_cleanup();
    return(NULL);
}