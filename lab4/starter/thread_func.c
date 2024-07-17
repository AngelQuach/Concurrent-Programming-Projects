#include "thread_func.h"

/* Modified from starter code; Process the url data */
/**
 * @brief process teh download data by curl
 * @param CURL *curl_handle is the curl handler
 * @param RECV_BUF p_recv_buf contains the received data. 
 * @return 0 on success; non-zero otherwise
 */
int process_data(CircularQueue *q, CURL *curl_handle, RECV_BUF *p_recv_buf)
{
    CURLcode res;
    long response_code;

    res = curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code);
    if ( res == CURLE_OK ) {
        /* Check response code */
        /* If response code < 400 -> Valid link */
        if(max(response_code, 400) < 400){
            /* Check type field */
                char *ct = NULL;
                res = curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_TYPE, &ct);
                if (res == CURLE_OK && ct != NULL) {
                    /* TESTING */
    	            printf("Content-Type: %s, len=%ld\n", ct, strlen(ct));

                    /* Check if it is HTML or PNG */
                    if(strstr(ct, CT_HTML)){        /* If is HTML */
                        /* Collect urls and add to frontier */
                        process_html(q, curl_handle, p_recv_buf);        
                    } else if(strstr(ct, CT_PNG)){
                        /* Check url is a valid png */
                        process_png(curl_handle, p_recv_buf); 
                    } else {
                        fprintf(stderr, "Failed obtain Content-Type\n");
                        return(2);
                    }
                }
        }
        /* Otherwise, we have broken link -> already handled */
    }
    return 0;
}

/* Modified from starter code; ensure proper handling of png url */
int process_png(CURL *curl_handle, RECV_BUF *p_recv_buf)
{
    char *eurl = NULL;          /* effective URL */
    curl_easy_getinfo(curl_handle, CURLINFO_EFFECTIVE_URL, &eurl);
    if (eurl != NULL) {
        /* Check the header for the received buffer */
        char buf[8];
        memcpy(p_recv_buf->buf, buf, 8);
        if(buf[1] == 0x50 && buf[2] == 0x4E && buf[3] == 0x47){        /* Check if file is a png */
            /* Add to png list */
        }
    }
    return 0;
}

/* Modified from starter code; ensure proper handling of html url */
int process_html(CircularQueue *q, CURL *curl_handle, RECV_BUF *p_recv_buf)
{
    int follow_relative_link = 1;
    char *url = NULL; 

    curl_easy_getinfo(curl_handle, CURLINFO_EFFECTIVE_URL, &url);
    find_http(q, p_recv_buf->buf, p_recv_buf->size, follow_relative_link, url); 
    return 0;
}

/* Modified from starter code; ensure all urls from http is copied to array */
int find_http(CircularQueue *q, char *buf, int size, int follow_relative_links, const char *base_url)
{
    int i;
    htmlDocPtr doc;
    xmlChar *xpath = (xmlChar*) "//a/@href";
    xmlNodeSetPtr nodeset;
    xmlXPathObjectPtr result;
    xmlChar *href;
		
    if (buf == NULL) {
        return 1;
    }

    doc = mem_getdoc(buf, size, base_url);
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
                /* TESTING */
                printf("href: %s\n", href);

                /* Add url to the url_frontiers array */
                while(enqueue(q, (char *)href) != 0);       /* Busy calling enqueue till url is added to array */
            }
            xmlFree(href);
        }
        xmlXPathFreeObject (result);
    }
    xmlFreeDoc(doc);
    xmlCleanupParser();
    return 0;
}