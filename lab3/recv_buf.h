#ifndef RECV_BUF_H
#define RECV_BUF_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>

#define uncomp_strip 9606  /* size of individual uncompressed strip */
#define uncomp_img 480300  /* total size of uncompressed strips */

/* Buffer for storing image segments */
typedef struct{
    char *buf;
    size_t size;
    size_t max_size;
    int seq;
} recv_buf;

recv_buf* init_image_buf();

#endif