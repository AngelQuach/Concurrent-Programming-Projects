#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "lab_png.h"
#include "crc.h"

int create_chunk(chunk_p *buf_chunk, const char *type_s, U32 width, U32 height, U8 *comp_png);
void write_chunk(FILE *fp, chunk_p buf_chunk);

int main(int argc, char *argv[])
{
    /* check if input is correct */
    if(argc < 2){
        printf("Usage: %s <png-file>\n", argv[0]);
        return -1;
    }

    /* length of the total uncompressed png */
    U64 totalDecomp_len = 0;
    /* width of each compressed png */
    U32 const_width;
    /* sum of png height before compression */
    U32 totalComp_height = 0;
    /* height of each decompressed png */
    U32 decomp_height[argc-1];

    /* buffer that stores pointers to the uncompressed IDAT */
    U8 **buf_alldecomp = (U8**)malloc((argc-1) * sizeof(U8 *));
    if (buf_alldecomp == NULL) {
        printf("Error allocating memory for buffer array\n");
        return -1;
    }

    /* For each file, we want to open it and read the IDAT chunk */
    for(int i = 1; i < argc; i++){
        FILE *f = fopen(argv[i], "rb");
        if(f == NULL){
		    printf("Unable to open file. %s is a invalid name?\n", argv[1]);
            buf_alldecomp[i-1] = NULL;
		    continue;
	    }

        /* Read IDAT compressed data length */
            U64 source_len;
            /* file pointer parameters */
            long offset = PNG_SIG_SIZE + CHUNK_LEN_SIZE + CHUNK_TYPE_SIZE;
            int whence = SEEK_SET;
            /* the compressed height read */
            U32 comp_height;
            /* if errors occur when reading IDAT length, move on to next file */
            if(read_IDAT_size(&source_len, &comp_height, &const_width, offset, whence, f) != 0){
                printf("Error reading IDAT data length for file %s\n", argv[i]);
                buf_alldecomp[i-1] = NULL;
                fclose(f);
                continue;
            }

        /* Read IDAT data */
            /* move file pointer to the start of data part of IDAT chunck*/
            offset = CHUNK_TYPE_SIZE;
            whence = SEEK_CUR;

            /* buffer that stores the compressed IDAT */
            U8 *buf_comp = NULL;
            /* read the compressed data of IDAT chunk to buf_comp*/
            if(read_IDAT_data(&buf_comp, source_len, CHUNK_TYPE_SIZE, SEEK_CUR, f) != 0){
                printf("Error reading IDAT data for file %s\n", argv[i]);
                buf_alldecomp[i-1] = NULL;
                fclose(f);
                continue;
            }

        /* Decompress the data of IDAT chunk and store it in buf_alldecomp */
            U64 decomp_len = (comp_height) * ((const_width) * 4 + 1);
            U8 *decomp_dest = (U8 *)malloc(decomp_len);
            if (decomp_dest == NULL) {
                printf("Error allocating memory for decompressed data for file %s\n", argv[i]);
                free(buf_comp);
                buf_alldecomp[i - 1] = NULL;
                fclose(f);
                continue;
            }
            memset(decomp_dest, 0, decomp_len);

            /* decompress data to decomp_dest */
            int return_val = mem_inf(decomp_dest, &decomp_len, buf_comp, source_len);
            if (return_val != Z_OK) {
                printf("Decompression failed for file %s\n", argv[i]);
                free(buf_comp);
                free(decomp_dest);
                buf_alldecomp[i-1] = NULL;
                fclose(f);
                continue;
            }

        /* update sum of height before compression */
            totalComp_height += comp_height;
        /* update array of pointers to the decompressed png */
            buf_alldecomp[i-1] = decomp_dest;
        /* update sum of height after compression and store individual individual height */
            totalDecomp_len += decomp_len;
            decomp_height[i-1] = decomp_len;

        free(buf_comp);
        fclose(f);
    }

    /* Concatenate all decompressed PNG into one buffer */
        U8 *buf_catDecomp = malloc(totalDecomp_len);
        if(buf_catDecomp == NULL){
            /* Free all allocated memory */
            for(int i = 0; i < (argc - 1); i++){
                if(buf_alldecomp[i] != NULL){
                    free(buf_alldecomp[i]);
                }
            }
            free(buf_alldecomp);
            return -1;
        }
        U64 offset = 0;
        for(int i = 0; i < argc-1; i++){
            if(buf_alldecomp[i] != NULL){
                memcpy(buf_catDecomp + offset, buf_alldecomp[i], decomp_height[i]);
                offset += decomp_height[i];
            }
        }

    /* Compressed the concatenated png file */
        /* total length of compressed png */
        U64 totalComp_len;
        /* buffer that stores the compressed png */
        U8 *buf_allcomp = (U8*)malloc(compressBound(totalDecomp_len));
        if(buf_allcomp == NULL){
            printf("Error allocating memory for buf_allcomp\n");
            /* Free all allocated memory */
            for(int i = 0; i < (argc - 1); i++){
                if(buf_alldecomp[i] != NULL){
                    free(buf_alldecomp[i]);
                }
            }
            free(buf_alldecomp);
            free(buf_catDecomp);
            return -1;
        }
        if(mem_def(buf_allcomp, &totalComp_len, buf_catDecomp, totalDecomp_len, Z_DEFAULT_COMPRESSION) != Z_OK){
            printf("Error compressing png files\n");
        }
    
    /* Create the new png file */
        /* pointer to all.png */
        simple_PNG_p all_png = malloc(sizeof(struct simple_PNG));
        if(all_png == NULL){
            printf("Error allocating memory for compressed file\n");
            free(buf_alldecomp);
            free(buf_catDecomp);
            free(buf_allcomp);
            return -1;
        }
        /* Set up IHDR chunk */
            if(create_chunk(&all_png->p_IHDR, "IHDR", const_width, totalComp_height, buf_allcomp) != 0){
                printf("Error allocating memory for IHDR chunk for new file\n");
                free(all_png);
                free(buf_alldecomp);
                free(buf_catDecomp);
                free(buf_allcomp);
                return -1;
            }
        /* IHDR completed */
        /* Set up IDAT chunk */
            if(create_chunk(&all_png->p_IDAT, "IDAT", (U32)totalComp_len, 0, buf_allcomp) != 0){
                printf("Error allocating memory for IDAT chunk for new file\n");
                free(all_png->p_IHDR->p_data);
                free(all_png->p_IHDR);
                free(all_png);
                free(buf_alldecomp);
                free(buf_catDecomp);
                free(buf_allcomp);
                return -1;
            }
        /* IDAT completed */
        /* Set up IEND chunk */
            if(create_chunk(&all_png->p_IEND, "IEND", 0, 0, buf_allcomp) != 0){
                printf("Error allocating memory for IEND chunk for new file\n");
                free(all_png->p_IHDR->p_data);
                free(all_png->p_IHDR);
                free(all_png->p_IDAT->p_data);
                free(all_png->p_IDAT);
                free(all_png);
                free(buf_alldecomp);
                free(buf_catDecomp);
                free(buf_allcomp);
                return -1;
            }
        /* IEND completed */

    /* Write the png file into a new file */
        /* create the png file to write to */
        FILE *f = fopen("all.png", "wb");
        if(f == NULL){
            printf("Failed to open a file for writing\n");
            free(all_png->p_IHDR->p_data);
            free(all_png->p_IHDR);
            free(all_png->p_IDAT->p_data);
            free(all_png->p_IDAT);
            free(all_png);
            free(buf_alldecomp);
            free(buf_catDecomp);
            free(buf_allcomp);
            return -1;
        }

        /* Write chunks to the file */
            /* header */
            const U8 png_header[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
            fwrite(&png_header, 1, sizeof(png_header), f);
            /* IHDR chunk */
            write_chunk(f, all_png->p_IHDR);
            /* IDAT chunk */
            write_chunk(f, all_png->p_IDAT);
            /* IEND chunk */
            write_chunk(f, all_png->p_IEND);

            fclose(f);    

    /* Free all allocated memory */
    for(int i = 0; i < (argc - 1); i++){
        if(buf_alldecomp[i] != NULL){
            free(buf_alldecomp[i]);
        }
    }

    free(all_png->p_IHDR->p_data);
    free(all_png->p_IHDR);
    free(all_png->p_IDAT->p_data);
    free(all_png->p_IDAT);
    free(all_png->p_IEND);
    free(all_png);
    free(buf_alldecomp);
    free(buf_catDecomp);

    return 0;
}

/* read data length of IDAT chunk into out */
int read_IDAT_size(U64 *out, U32 *height, U32 *width, long offset, int whence, FILE *fp){
    /* calculate size of IDAT data chunck */
	    /* create IHDR chunk for the png */
	    struct data_IHDR ihdr;
	    if(get_png_data_IHDR(&ihdr, fp, offset, whence) != 0) return -1;

        /* data length of the IDAT chunk */
	    *height = ihdr.height;
        *width = ihdr.width;

    /* read the length of compressed data */
        if(fseek(fp, CHUNK_CRC_SIZE, SEEK_CUR) != 0) return -1;

        /* temporary variable to store data length of png */
        U32 temp_len;
        if(fread(&temp_len, CHUNK_LEN_SIZE, 1, fp) != 1) return -1;

        *out = ntohl(temp_len);
        return 0;
}

/* read compressed IDAT data into (the address pointed to by U8 *out) */
int read_IDAT_data(U8 **out, U64 data_length, long offset, int whence, FILE *fp){
    /* a pointer to the address storing the compressed data of IDAT chunk */
    *out = (U8*)malloc(data_length);
    if(*out == NULL){
        printf("Error allocating memory for IDAT data\n");
        return -1;
    }
    memset(*out, 0, data_length);

    /* move file pointer to the start of data part of IDAT chunck*/
    if(fseek(fp, offset, whence) != 0){
		/* for handling error */
        printf("Error moving file point to data chunk of IDAT chunk\n");
        free(*out);
		return -1;
	}

    /* read data part */
    if(fread(*out, 1, data_length, fp) != data_length){
        printf("Error reading IDAT data\n");
        free(*out);
        return -1;
    }
    return 0;
}

/* obtain the IHDR data of the png file */
int get_png_data_IHDR(struct data_IHDR *out, FILE *fp, long offset, int whence){
	/* set up fread by moving the file pointer */
	if(fseek(fp, offset, whence) != 0){
		/* for handling error */
		return -1;
	}
	/* read width */
	size_t elementsRead = fread(&(out->width), sizeof(U32), 1, fp);
	if(elementsRead != 1){
		return -1;
	}
	/* read height */
	elementsRead = fread(&(out->height), sizeof(U32), 1, fp);
	if(elementsRead != 1){
        return -1;
    }
	/* read bit_depth */
	elementsRead = fread(&(out->bit_depth), sizeof(U8), 1, fp);
	if(elementsRead != 1){
        return -1;
    }
	/* read color_type */
	elementsRead = fread(&(out->color_type), sizeof(U8), 1, fp);
	if(elementsRead != 1){
        return -1;
    }
	/* read compression, filter, interlace */
	elementsRead = fread(&(out->compression), sizeof(U8), 1, fp);
	if(elementsRead != 1){
        return -1;
    }
	elementsRead = fread(&(out->filter), sizeof(U8), 1, fp);
	if(elementsRead != 1){
        return -1;
    }
	elementsRead = fread(&(out->interlace), sizeof(U8), 1, fp);
	if(elementsRead != 1){
        return -1;
    }

    /* convert width and height into little-endian */
    out->height = ntohl(out->height);
    out->width = ntohl(out->width);

	return 0;
}

/* initiate and set up the chunk of type_s */
int create_chunk(chunk_p *buf_chunk, const char *type_s, U32 width, U32 height, U8 *comp_png){
    /* pointer to chunk created */
    *buf_chunk = malloc(sizeof(struct chunk));
    if(buf_chunk == NULL) return -1;
    
    /* determine which chunk we are creating */
    if(!strcmp(type_s, "IHDR")){
        /* IHDR length and type */
            (*buf_chunk)->length = DATA_IHDR_SIZE;
            memcpy((*buf_chunk)->type, type_s, 4);

        /* IHDR data */
            data_IHDR_p IHDR_d = malloc(DATA_IHDR_SIZE);
            if(IHDR_d == NULL){
                free(buf_chunk);
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
        free(*buf_chunk);
        return -1;
    }

    /* Set up chunk crc */
        U8 *temp_buf = malloc(CHUNK_TYPE_SIZE + (*buf_chunk)->length);
        if(temp_buf == NULL){
            if(!strcmp(type_s, "IHDR")){
                free((*buf_chunk)->p_data);
            }
            free(*buf_chunk);
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