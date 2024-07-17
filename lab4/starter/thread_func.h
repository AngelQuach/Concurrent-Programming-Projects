#ifndef THREAD_FUNC_H
#define THREAD_FUNC_H

#include "main.h"
#include "data_structure.h"

int process_data(CircularQueue *q, CURL *curl_handle, RECV_BUF *p_recv_buf);
int process_png(CURL *curl_handle, RECV_BUF *p_recv_buf);
int process_html(CircularQueue *q, CURL *curl_handle, RECV_BUF *p_recv_buf);
int find_http(CircularQueue *q, char *buf, int size, int follow_relative_links, const char *base_url);

#endif