#include <curl/curl.h>
#include "common.h"

#define ECE252_HEADER "X-Ece252-Fragment: "

/* The below codes are from lab 3 starter code */
/**
 * @brief  cURL header call back function to extract image sequence number from 
 *         http header data. An example header for image part n (assume n = 2) is:
 *         X-Ece252-Fragment: 2
 * @param  char *p_recv: header data delivered by cURL
 * @param  size_t size size of each memb
 * @param  size_t nmemb number of memb
 * @param  void *userdata user defined data structurea
 * @return size of header data received.
 * @details this routine will be invoked multiple times by the libcurl until the full
 * header data are received.  we are only interested in the ECE252_HEADER line 
 * received so that we can extract the image sequence number from it. This
 * explains the if block in the code.
 */
size_t header_cb_curl(char *p_recv, size_t size, size_t nmemb, void *userdata)
{
    int realsize = size * nmemb;
    recv_buf *p = userdata;
    
    if (realsize > strlen(ECE252_HEADER) &&
	    strncmp(p_recv, ECE252_HEADER, strlen(ECE252_HEADER)) == 0) {
        /* extract img sequence number */
	    p->seq = atoi(p_recv + strlen(ECE252_HEADER));
    }
    return realsize;
}

/**
 * @brief write callback function to save a copy of received data in RAM.
 *        The received libcurl data are pointed by p_recv, 
 *        which is provided by libcurl and is not user allocated memory.
 *        The user allocated memory is at p_userdata. One needs to
 *        cast it to the proper struct to make good use of it.
 *        This function maybe invoked more than once by one invokation of
 *        curl_easy_perform().
 */
size_t write_cb_curl(char *p_recv, size_t size, size_t nmemb, void *p_userdata)
{
    size_t realsize = size * nmemb;
    recv_buf *p = (recv_buf *)p_userdata;
 
    if (p->size + realsize + 1 > p->max_size) {/* hope this rarely happens */ 
        fprintf(stderr, "User buffer is too small, abort...\n");
        abort();
    }

    memcpy(p->buf + p->size, p_recv, realsize); /*copy data from libcurl*/
    p->size += realsize;
    p->buf[p->size] = 0;

    return realsize;
}

/* The below code is modified from lab 2 paster.c */
void producer_process(CircularQueue *queue, int machine_num, int img_num) {
    CURL *curl_handle;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl_handle = curl_easy_init();
    if (curl_handle) {
        /* init the recv_buf */
        recv_buf *img = init_image_buf();
        if(img == NULL){
            perror("Error creating buffer for CURL");
            curl_easy_cleanup(curl_handle);
            curl_global_cleanup();
            exit(EXIT_FAILURE);
        }

        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_cb_curl);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, img);
        curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_cb_curl);
        curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, img);
        curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

        /* Check if all strips are downloaded already */
        while(1){
            /* Set up url to access */
            sem_wait(&queue->access_sem);
                if(queue->last_seq >= MAX_STRIPS){
                    sem_post(&queue->access_sem);
                    break;
                }
                char url[256];
                snprintf(url, sizeof(url), "http://ece252-%d.uwaterloo.ca:2530/image?img=%d&part=%d", machine_num, img_num, queue->last_seq);
                queue->last_seq++;
            sem_post(&queue->access_sem);
            curl_easy_setopt(curl_handle, CURLOPT_URL, url);

            /* Request strip */
            res = curl_easy_perform(curl_handle);
            if (res != CURLE_OK) {
                fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            }

            /* Check if the array is full */
            sem_wait(&queue->empty);
            /* Check if another producer process is in critical section */
            sem_wait(&queue->prod_sem);

            /* If this strips has not been downloaded, download it */
            if (img->seq >= 0 && queue->last_seq <= MAX_STRIPS) {
                /* Add the provided buffer to the array */
                queue->buf[queue->in] = img;
                /* Increment count of recv_buf in the array */
                queue->num++;
                queue->in = (queue->in + 1)%(queue->capacity);
            } else {
                if(img->buf != NULL){
                    free(img->buf);
                }
                free(img);
                img = NULL;
            }

            /* Decrement full */
            sem_post(&queue->full);
            /* Exit critical section */
            sem_post(&queue->prod_sem);
        }

        /* After all strips have been downloaded, clean curl handle */
        curl_easy_cleanup(curl_handle);
        curl_global_cleanup();
    } else {
        /* In case curl_easy_init() failed */
        curl_global_cleanup();
    }
}
