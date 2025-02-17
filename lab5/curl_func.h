#ifndef CURL_FUNC_H
#define CURL_FUNC_H

#include "main.h"
#include "data_structure.h"

int process_data(CircularQueue *q, URL_Array *PNG_url, HashTable *url_visited, CURL *curl_handle, RECV_BUF *p_recv_buf);
int process_png(CURL *curl_handle, URL_Array *PNG_url, RECV_BUF *p_recv_buf);
int process_html(CircularQueue *q, HashTable *url_visited, CURL *curl_handle, RECV_BUF *p_recv_buf);
int find_http(CircularQueue *q, HashTable *url_visited, char *buf, int size, int follow_relative_links, const char *base_url);

#endif