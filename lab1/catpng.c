#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "lab_png.h"
#include "zutil.h"

int read_IDAT_size(U32 *out, U32 *height, U32 *width, long offset, int whence, FILE *fp);
int read_IDAT_data(U8 **out, U64 data_length, long offset, int whence, FILE *fp);

int main(int argc, char *argv[])
{
    /* check if input is correct */
    if(argc < 2){
        printf("Usage: %s <png-file>\n", argv[0]);
        return -1;
    }

    /* buffer that stores pointers to the uncompressed IDAT */
    U8 **buf_alluncomp = (U8**)malloc((argc-1) * sizeof(U8 *));
    if (buf_alluncomp == NULL) {
        printf("Error allocating memory for buffer array\n");
        return -1;
    }

    /* For each file, we want to open it and read the IDAT chunk */
    for(int i = 1; i < argc; i++){
        FILE *f = fopen(argv[i], "rb");
        if(f == NULL){
		    printf("Unable to open file. %s is a invalid name?\n", argv[1]);
            buf_alluncomp[i-1] = NULL;
		    continue;
	    }

        /* Read IDAT compressed data length */
            U32 *source_width = malloc(sizeof(U32));
            U32 *source_height = malloc(sizeof(U32));
            U64 *source_length = malloc(sizeof(U64));
            if(source_length == NULL || source_width == NULL || source_height == NULL){
                printf("Unable to allocate memory\n");
                buf_alluncomp[i-1] = NULL;
                free(source_length);
                free(source_width);
                free(source_width);
                buf_alluncomp[i-1] = NULL;
                fclose(f);
                continue;
            }

            /* file pointer parameters */
            long offset = PNG_SIG_SIZE + CHUNK_LEN_SIZE + CHUNK_TYPE_SIZE;
            int whence = SEEK_SET;
            /* temporary store data length of IDAT */
            U32 temp_len;
            /* if errors occur when reading IDAT length, move on to next file */
            if(read_IDAT_size(&temp_len, source_height, source_width, offset, whence, f) != 0){
                printf("Error reading IDAT data length for file %s\n", argv[i]);
                free(source_length);
                free(source_width);
                free(source_height);
                buf_alluncomp[i-1] = NULL;
                fclose(f);
                continue;
            }
            *source_length = ntohl(temp_len);

        /* Read IDAT data */
            /* move file pointer to the start of data part of IDAT chunck*/
            offset = CHUNK_TYPE_SIZE;
            whence = SEEK_CUR;

            /* buffer that stores the compressed IDAT */
            U8 *buf_comp = NULL;
            /* read the compressed data of IDAT chunk to buf_comp*/
            if(read_IDAT_data(&buf_comp, *source_length, CHUNK_TYPE_SIZE, SEEK_CUR, f) != 0){
                printf("Error reading IDAT data for file %s\n", argv[i]);
                free(source_length);
                free(source_width);
                free(source_height);
                buf_alluncomp[i-1] = NULL;
                fclose(f);
                continue;
            }

        /* decompress the data of IDAT chunk and store it in buf_alluncomp */
            U64 decomp_len = (*source_height)* ((*source_width)*4+1);
            U8 *decomp_dest = (U8 *)malloc(decomp_len);
            if (decomp_dest == NULL) {
                printf("Error allocating memory for decompressed data for file %s\n", argv[i]);
                free(source_length);
                free(source_width);
                free(source_height);
                free(buf_comp);
                buf_alluncomp[i - 1] = NULL;
                fclose(f);
                continue;
            }
            memset(decomp_dest, 0, decomp_len);

            /* decompress data to decomp_dest */
            int return_val = mem_inf(decomp_dest, &decomp_len, buf_comp, *source_length);
            if (return_val != Z_OK) {
                printf("Decompression failed for file %s\n", argv[i]);
                free(source_length);
                free(source_width);
                free(source_height);
                free(buf_comp);
                free(decomp_dest);
                buf_alluncomp[i-1] = NULL;
                fclose(f);
                continue;
            }

            printf("Decompressed length: %lu\n", decomp_len);

        /* stores the decompressed png in the array */
            buf_alluncomp[i-1] = decomp_dest;

        free(source_length);
        free(source_width);
        free(source_height);
        free(buf_comp);
        fclose(f);
    }

    /* Free all allocated memory */
    for(int i = 0; i < (argc - 1); i++){
        if(buf_alluncomp[i] != NULL){
            free(buf_alluncomp[i]);
        }
    }
    free(buf_alluncomp);

    return 0;
}

/* read data length of IDAT chunk into out */
int read_IDAT_size(U32 *out, U32 *height, U32 *width, long offset, int whence, FILE *fp){
    /* calculate size of IDAT data chunck */
	    /* create IHDR chunk for the png */
	    struct data_IHDR *ihdr = malloc(DATA_IHDR_SIZE);
	    if(ihdr == NULL){
		    return -1;
	    }
	    memset(ihdr, 0, DATA_IHDR_SIZE);
	    if(get_png_data_IHDR(ihdr, fp, offset, whence) != 0){
		    free(ihdr);
		    return -1;
	    }

        /* data length of the IDAT chunk */
	    *height = ihdr->height;
        *width = ihdr->width;
	    free(ihdr);

    /* read the length of compressed data */
        if(fseek(fp, CHUNK_CRC_SIZE, SEEK_CUR) != 0){
            printf("Error moving file pointer to IDAT chunk\n");
            return -1;
        }
        int elementsRead = fread(out, CHUNK_LEN_SIZE, 1, fp);
        if(elementsRead != 1){
            printf("Error: fail to read IDAT length chunk\n");
            return -1;
        }
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
    int elementsRead = fread(*out, 1, data_length, fp);
    if(elementsRead != data_length){
        printf("Error reading IDAT data: Expected: %lu, Actual: %d\n", data_length, elementsRead);
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