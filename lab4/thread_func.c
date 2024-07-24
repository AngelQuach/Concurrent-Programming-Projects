#include "thread_func.h"

/* Modified from starter code; Process the url data */
/**
 * @brief process teh download data by curl
 * @param CURL *curl_handle is the curl handler
 * @param RECV_BUF p_recv_buf contains the received data. 
 * @return 0 on success; non-zero otherwise
 */
int process_data(CircularQueue *q, URL_Array *PNG_url, pthread_mutex_t mutex_PNG, pthread_mutex_t mutex_HTTP, HashTable *url_visited, CURL *curl_handle, RECV_BUF *p_recv_buf)
{
    CURLcode res;
    long response_code;

    /* Get information about response type */
    res = curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code);

    /* If this is a broken link, do nothing */
    if ( response_code >= 400 ) { 
        return 0;
    }

    char *ct = NULL;
    res = curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_TYPE, &ct);
    if (!(res == CURLE_OK && ct != NULL)) {
        fprintf(stderr, "Failed obtain Content-Type\n");
        return 2;
    }

    /* Check if it is a HTML or a PNG */
    if ( strstr(ct, CT_HTML) ) {
        return process_html(q, url_visited, mutex_HTTP, curl_handle, p_recv_buf);
    } else if ( strstr(ct, CT_PNG) ) {
        return process_png(curl_handle, PNG_url, mutex_PNG, p_recv_buf);
    }
    return 0;
}

/* Modified from starter code; ensure proper handling of png url */
int process_png(CURL *curl_handle, URL_Array *PNG_url, pthread_mutex_t mutex, RECV_BUF *p_recv_buf)
{
    char *eurl = NULL;          /* effective URL */
    curl_easy_getinfo(curl_handle, CURLINFO_EFFECTIVE_URL, &eurl);
    if (eurl != NULL) {
        /* Check the header for the received buffer */
        char buf[8];
        memcpy(buf, p_recv_buf->buf, 8);
        if(buf[1] == 0x50 && buf[2] == 0x4E && buf[3] == 0x47){        /* Check if file is a png */
            pthread_mutex_lock(&mutex);
                /* Add to png list */
                while(countPNGURL(PNG_url) < PNG_url->PNG_needed && addPNGURL(PNG_url, eurl) != 0);                      /* Busy adding url to the list */
            pthread_mutex_unlock(&mutex);
        }
        return 0;
    } else{
        return 1;
    }
}

/* Modified from starter code; ensure proper handling of html url */
int process_html(CircularQueue *q, HashTable *url_visited, pthread_mutex_t mutex, CURL *curl_handle, RECV_BUF *p_recv_buf)
{
    int follow_relative_link = 1;
    char *url = NULL; 

    curl_easy_getinfo(curl_handle, CURLINFO_EFFECTIVE_URL, &url);
    find_http(q, url_visited, mutex, p_recv_buf->buf, p_recv_buf->size, follow_relative_link, url); 
    return 0;
}

/* Modified from starter code; ensure all urls from http is copied to array */
int find_http(CircularQueue *q, HashTable *url_visited, pthread_mutex_t mutex, char *buf, int size, int follow_relative_links, const char *base_url)
{
    int i;
    htmlDocPtr doc = NULL;
    xmlChar *xpath = (xmlChar*) "//a/@href";
    xmlNodeSetPtr nodeset;
    xmlXPathObjectPtr result = NULL;
    xmlChar *href;
		
    if (buf == NULL) {
        return 1;
    }

    doc = mem_getdoc(buf, size, base_url);
    if (doc == NULL) {
        return 1;
    }
    result = getnodeset (doc, xpath);
    if (result) {
        nodeset = result->nodesetval;
        for (i=0; i < nodeset->nodeNr; i++) {
            href = xmlNodeListGetString(doc, nodeset->nodeTab[i]->xmlChildrenNode, 1);
            if (follow_relative_links) {
                xmlChar *old = href;
                href = xmlBuildURI(href, (xmlChar *) base_url);
                xmlFree(old);
            }
            if (href != NULL && !strncmp((const char *)href, "http", 4)) {
                /* Check if url has been added */
                pthread_mutex_lock(&mutex);
                if(checkHashURL(url_visited, (char *)href) == 0){
                    /* If not visited add to url visited array */
                    addHashURL(url_visited, (char *)href);

                    /* Add url to the url_frontiers array */
                    while(enqueue(q, (char *)href) != 0);       /* Busy calling enqueue till url is added to array */
                }  /* If already added before, do nothing */
                pthread_mutex_unlock(&mutex);
            }
            xmlFree(href);
        }
        xmlXPathFreeObject (result);
    }
    xmlFreeDoc(doc);
    return 0;
}