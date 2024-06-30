#include "lab_png.h"

/* initiate and set up the chunk of type_s */
int create_chunk(chunk_p *buf_chunk, const char *type_s, U32 width, U32 height, U8 *comp_png){
    /* pointer to chunk created */
    (*buf_chunk) = malloc(sizeof(struct chunk));
    if((*buf_chunk) == NULL) return -1;
    
    /* determine which chunk we are creating */
    if(!strcmp(type_s, "IHDR")){
        /* IHDR length and type */
            (*buf_chunk)->length = DATA_IHDR_SIZE;
            memcpy((*buf_chunk)->type, type_s, 4);

        /* IHDR data */
            data_IHDR_p IHDR_d = malloc(DATA_IHDR_SIZE);
            if(IHDR_d == NULL){
                free((*buf_chunk));
                return -1;
            }
            memset(IHDR_d, 0, DATA_IHDR_SIZE);

            IHDR_d->width = htonl(width);
            IHDR_d->height = htonl(height);
            IHDR_d->bit_depth = 8;
            IHDR_d->color_type = 6;
            IHDR_d->compression = 0;
            IHDR_d->filter = 0;
            IHDR_d->interlace = 0;
            (*buf_chunk)->p_data = (void *) IHDR_d;
    } else if(!strcmp(type_s, "IDAT")){
        /* IDAT length and type */
            (*buf_chunk)->length = width;
            memcpy((*buf_chunk)->type, type_s, 4);

        /* IDAT data */
            (*buf_chunk)->p_data = comp_png;
    } else if(!strcmp(type_s, "IEND")){
        /* IEND length and type */
            (*buf_chunk)->length = 0;
            memcpy((*buf_chunk)->type, type_s, 4);
        
        /* IEND data */
            (*buf_chunk)->p_data = NULL;
    } else{
        /* Unsupported chunk type */
        free((*buf_chunk));
        return -1;
    }

    /* Set up chunk crc */
        U8 *temp_buf = malloc(CHUNK_TYPE_SIZE + (*buf_chunk)->length);
        if(temp_buf == NULL){
            if(!strcmp(type_s, "IHDR")){
                free((*buf_chunk)->p_data);
            }
            free((*buf_chunk));
            return -1;
        }
        memcpy(temp_buf, (*buf_chunk)->type, CHUNK_TYPE_SIZE);
        /* for IHDR/IDAT chunks */
        if((*buf_chunk)->length > 0 && (*buf_chunk)->p_data != NULL){
            memcpy(temp_buf + CHUNK_TYPE_SIZE, (*buf_chunk)->p_data, (*buf_chunk)->length);
        }
        (*buf_chunk)->crc = crc(temp_buf, CHUNK_TYPE_SIZE + (*buf_chunk)->length);
        free(temp_buf);
    
    /* chunk set up completed */
        return 0;
}

/* write buf_chunk to the file fp */
void write_chunk(FILE *fp, chunk_p buf_chunk){
    /* Write length and type */
    U32 chunk_len = htonl(buf_chunk->length);
    fwrite(&chunk_len, 1, CHUNK_LEN_SIZE, fp);
    fwrite(buf_chunk->type, 1, CHUNK_TYPE_SIZE, fp);

    /* Check if data exists, write data */
    if(buf_chunk->p_data != NULL){
        fwrite(buf_chunk->p_data, 1, buf_chunk->length, fp);
    }

    /* Write crc */
    U32 crc_value = htonl(buf_chunk->crc);
    fwrite(&crc_value, 1, CHUNK_CRC_SIZE, fp);
}