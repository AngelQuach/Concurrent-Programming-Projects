#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>

#define uncomp_strip 9606  /* size of individual uncompressed strip */
#define uncomp_img 480300  /* total size of uncompressed strips */

typedef struct recv_buf;
recv_buf* init_image_buf();