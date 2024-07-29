#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "data_structure.h"     /* For data strucutres */
#include "curl_func.h"          /* For data_processing functions */
#include "main.h"               /* For CURL functions */
#include <curl/multi.h>         /* For CURL multi-interface */

#define MAX_URLS 500
#define TABLE_SIZE 500
#define MAX_PNG_URLS 500
#define URL_SIZE 256
#define BUF_SIZE 1048576              /* 1024*1024 = 1M */
#define MAX_WAIT_MSECS 10*1000        /* Wait max. 10 seconds */
#define MAX_RETRIES 3

CircularQueue url_frontiers;          /* url_Frontiers array for storing all urls */
HashTable url_visited;                /* url_visited array for storing visited urls */
URL_Array PNG_url;                    /* PNG_url array for storing identified PNG urls */

/* Define default variables */
int num_handle = 1;
int num_unique_PNG = 50;
int if_logfile = 0;
char *log_filename = NULL;
char *input_url = NULL;
int still_running = 0;
int active_handle_count = 0;
CURL **all_handle = NULL;
int handle_count = 0;
int waiting_handle_count = 0;
char **all_urls = NULL;
int urls_count = 0;

/* Declare functions here */
void init(CURLM *cm);
void *handle_url(CURLM *cm);

int main(int argc, char* argv[]) 
{
    /* Get input from the user */
    int opt;
    while ((opt = getopt(argc, argv, "t:m:v:")) != -1) {
        switch (opt) {
            case 't':
                if(optarg != NULL && *optarg != '\0'){
                    num_handle = strtoul(optarg, NULL, 10);
                    if(num_handle <= 0){
                        fprintf(stderr, "Number of concurrent connection must be at least 1\n");
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
        initArray(&PNG_url, num_unique_PNG);

    /*Set up an array to store all urls used by CURL*/
        all_urls = malloc(MAX_URLS*sizeof(char *));
        for(int i = 0; i < MAX_URLS; i++){
            all_urls[i] = NULL;
        }
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

    /* Var. for curl */
        all_handle = malloc(num_handle*sizeof(CURL*));
        if(all_handle == NULL){
            perror("Failed to allocate memory for all_handle");
            return 1;
        }
        for(int i = 0; i < num_handle; i++){        /* Initialize the array */
            all_handle[i] = NULL;
        }
        CURLM *cm = NULL;
        /* Initialize connections */
        curl_global_init(CURL_GLOBAL_ALL);
        cm = curl_multi_init();
        /* Initialize the first handle */
        init(cm);
        handle_url(cm);

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
            for(int i = 0; i < url_visited.count; i++){
                if(url_visited.url[i] != NULL){
                    fprintf(f_output, "%s\n", url_visited.url[i]);
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
    for(int i = 0; i < urls_count; i++){
        free(all_urls[i]);
    }
    free(all_urls);
    free(input_url);
    freeQueue(&url_frontiers);
    freeHashTable(&url_visited);
    freeArray(&PNG_url);
    free(all_handle);
    curl_multi_cleanup(cm);
    curl_global_cleanup();
    xmlCleanupParser();   

    return 0;
}

/* Function to init CURL handles */
void init(CURLM *cm){
    CURL *eh = NULL;
    RECV_BUF *recv_buf = malloc(sizeof(RECV_BUF));
    if(recv_buf == NULL){
        perror("Failed to allocate memory for recv_buf");
        exit(EXIT_FAILURE);
    }
    char *url = NULL;       
    url = dequeue(&url_frontiers, &num_handle);
    if(url != NULL){
        eh = easy_handle_init(recv_buf, url);
        if(eh != NULL){
            /* Search for an empty entry to add new handle */
            all_handle[handle_count] = eh;
            handle_count++;
            curl_multi_add_handle(cm, eh);
            active_handle_count++;

            /* Keep track of the urls used */
            all_urls[urls_count] = url;
            urls_count++;
        } else{
            recv_buf_cleanup(recv_buf);
            free(recv_buf);
            free(url);
            abort();
        }
    } else{
        free(recv_buf);
    }
}

/* Function for connection to handle url */
void *handle_url(CURLM *cm){
    int retry_count = 0;
    CURL *eh;

    /* Check if all needed PNG is fetched */
    while(countPNGURL(&PNG_url) < num_unique_PNG || still_running || active_handle_count > 0){
        /* Start the multi_perform */
        curl_multi_perform(cm, &still_running);

        /* Wait for data to be transferred */
        int numfds = 0;
        int res = curl_multi_wait(cm, NULL, 0, MAX_WAIT_MSECS, &numfds);
        if(res != CURLM_OK) {
            // fprintf(stderr, "error: curl_multi_wait() returned %d\n", res);
            return (void *)EXIT_FAILURE;
        }

        /* Check returned msg */
        CURLMsg *msg;
        int msgs_left = 0;
        while ((msg = curl_multi_info_read(cm, &msgs_left))) {
            if(msg->msg == CURLMSG_DONE) {                      /* If the current CURL request has finished */
                RECV_BUF *recv_buf = NULL;                      /* The received data from URL */
                eh = msg->easy_handle;                          /* Get the handle of the completed CURL request */
                CURLcode res = msg->data.result;                /* Get the response code */

                if(res == CURLE_OK){                            /* If the request completed successfully */
                    /* Get info for the handle */
                    curl_easy_getinfo(eh, CURLINFO_PRIVATE, (void **)&recv_buf);
                    if(recv_buf != NULL){
                        if (recv_buf->size != 0) {
                            process_data(&url_frontiers, &PNG_url, &url_visited, eh, recv_buf);
                        }
                        recv_buf_cleanup(recv_buf);
                        retry_count = 0;                        /* Reset retry count on success */
                    }
                } else{                                         /* Else print error message */
                    if (res == CURLE_COULDNT_CONNECT && retry_count < MAX_RETRIES) {
                        retry_count++;
                        // fprintf(stderr, "Retrying connection, attempt %d of %d\n", retry_count, MAX_RETRIES);
                        curl_multi_remove_handle(cm, eh);
                        curl_multi_add_handle(cm, eh);
                        continue;
                    }
                }

                /* Remove handle from the multi-handle */
                curl_multi_remove_handle(cm, eh);
                active_handle_count--;

                if(countPNGURL(&PNG_url) == num_unique_PNG || url_frontiers.count == 0){
                    break;
                }

                /* Check if we need to process new urls */
                if(url_frontiers.count > 0){
                    /* specify URL to get */
                    char *next_url = dequeue(&url_frontiers, &num_handle);
                    curl_easy_setopt(eh, CURLOPT_URL, next_url);

                    /* Add handle back to multi */
                    curl_multi_add_handle(cm, eh);
                    active_handle_count++;

                    /* Keep track of the urls used */
                    all_urls[urls_count] = next_url;
                    urls_count++;

                    /* As long as there are still url left in frontier AND we can create more handles */
                    while(handle_count < (num_handle-1) && url_frontiers.count > 0){
                        /* Init and add new handle to multi */
                        init(cm);
                    }
                } else{
                    /* Remove the handle */
                    /* Free the recv_buf */
                        RECV_BUF *recv_buf2 = NULL;
                        curl_easy_getinfo(eh, CURLINFO_PRIVATE, (void **)&recv_buf2); 
                        if(recv_buf2 != NULL){
                            recv_buf_cleanup(recv_buf2);
                            free(recv_buf2);
                        }

                    /* Clean up the handle */
                        curl_easy_cleanup(eh);

                    /* Find handle in the array */
                        int found_index = -1;
                        for(int i = 0; i < handle_count; i++){
                            if(all_handle[i] == eh){
                                all_handle[i] = NULL;
                                found_index = i;
                            }
                            if(found_index != -1 && i > found_index){
                                all_handle[i-1] = all_handle[i];
                            }
                        }
                    /* Update handle count */
                        handle_count--;
                }
            }
        }
        numfds = 0;
        if(countPNGURL(&PNG_url) == num_unique_PNG){
            curl_multi_wait(cm, NULL, 0, MAX_WAIT_MSECS, &numfds);           /* Wait for all handles to finish */
            for(int i = 0; i < handle_count; i++){
                if(all_handle[i] != NULL && all_handle[i] != eh){            /* Detach all handles */
                    curl_multi_remove_handle(cm, all_handle[i]);
                    active_handle_count--;
                }
            }
            break;
        } else if(url_frontiers.count == 0 && active_handle_count == 0){
            break;
        }
    }
    /* Free all allocated memory for each handle */
    for(int i = 0; i < handle_count; i++){
        if(all_handle[i] != NULL){
            /* Free the recv_buf */
            RECV_BUF *recv_buf2 = NULL;
            curl_easy_getinfo(all_handle[i], CURLINFO_PRIVATE, (void **)&recv_buf2); 
            if(recv_buf2 != NULL){
                recv_buf_cleanup(recv_buf2);
                free(recv_buf2);
            }
            /* Clean up the handle */
            curl_easy_cleanup(all_handle[i]);
        }
    }
    return NULL;
}