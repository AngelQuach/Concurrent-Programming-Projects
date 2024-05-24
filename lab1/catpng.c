#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "lab_png.h"

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
    /* height of each compressed png, decompressed png */
    U32 comp_height[argc-1];
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
            /* if errors occur when reading IDAT length, move on to next file */
            if(read_IDAT_size(&source_len, &comp_height[i-1], &const_width, offset, whence, f) != 0){
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
            U64 decomp_len = (comp_height[i-1]) * ((const_width) * 4 + 1);
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

        /* stores the decompressed png in the array */
            buf_alldecomp[i-1] = decomp_dest;
        /* keep track of uncompressed length */
            totalDecomp_len += decomp_len;
            decomp_height[i-1] = decomp_len;

        free(buf_comp);
        fclose(f);
    }

    /* for debugging purpose */
    printf("Total decomp_len is: %lu\n", totalDecomp_len);

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

    /* Compressed all png files into one */
        /* total length of compressed png */
        U64 totalcomp_len;
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

        if(mem_def(buf_allcomp, &totalcomp_len, buf_catDecomp, totalDecomp_len, Z_DEFAULT_COMPRESSION) != Z_OK){
            printf("Error compressing png files\n");
        }

    /*  For debuggin purpose */
    printf("Compressed length: %lu\n", totalcomp_len);

    /* Free all allocated memory */
    for(int i = 0; i < (argc - 1); i++){
        if(buf_alldecomp[i] != NULL){
            free(buf_alldecomp[i]);
        }
    }
    free(buf_alldecomp);
    free(buf_catDecomp);
    free(buf_allcomp);

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