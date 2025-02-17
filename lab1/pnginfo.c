/**
 * @file: ECE252/lab1/pnginfo.c
 * @brief: Checks if the file is a PNG and output dimensions
 * @author: Angel Quach, Taylor Yin
 * @date: 2024/05/21
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "lab_png.h"
#include "crc.h"

U32 bigEnd_to_littleEnd(U32 bigEnd_val);

int main(int argc, char **argv){
	/* check if input is of correct format */
	if(argc != 2){
			printf("Format: pnginfo <filename>\n");
			return -1;
	}

	/* open the file specified */
	FILE *f = fopen(argv[1], "rb");
	/* check if the file exists */
	if(f == NULL){
		printf("Unable to open file. %s is a invalid name?\n", argv[1]);
		return -1;
	}

	/* We want to read 8-byte identifier from the header */
	U8 *header = malloc(8);
	size_t elementsRead = fread(header, sizeof(U8), 8, f);
	if(elementsRead != 8){
		printf("Error reading the file. Expected elements: 8. Actual elements: %ld\n", elementsRead);
		fclose(f);
		free(header);
		return -1;
	}
	/* check whether the file is a png */
	int png_state = is_png(header, 8);
	if(png_state == 1){
		/* create IHDR chunk for the png */
		struct data_IHDR *ihdr = malloc(DATA_IHDR_SIZE);
		if(ihdr == NULL){
			printf("Failed to allocate memory for IHDR\n");
			fclose(f);
			free(header);
			return -1;
		}
		memset(ihdr, 0, DATA_IHDR_SIZE);
				
		/* with header read, should move 8 bytes from the current pointer pos */
		if(get_png_data_IHDR(ihdr, f, (CHUNK_LEN_SIZE + CHUNK_TYPE_SIZE), SEEK_CUR) != 0){
			printf("Error reading IHDR chunk from: %s\n", argv[1]);
			fclose(f);
			free(ihdr);
			free(header);
			return -1;
		}
		int png_width = get_png_width(ihdr);
		int png_height = get_png_height(ihdr);
		printf("%s: %d x %d\n", argv[1], png_width, png_height);
		free(ihdr);

		/* check if crc value match */
		U32 *crc_p = malloc(sizeof(U32));
			/* IHDR crc check */
				/* read IHDR crc */
				elementsRead = fread(crc_p, CHUNK_CRC_SIZE, 1, f);
				/* compute expected IHDR crc */
				U32 expected_crc = crc(f, CHUNK_CRC_SIZE);
				if(elementsRead != 1){
					/* error handling */
					printf("Error: can't read the CRC of %s\n", argv[1]);
				}
				if(expected_crc != *crc_p){
					printf("IHDR chunk CRC error: computed %d, expected %d\n", *crc_p, expected_crc);
				}
			
			/* IDAT crc check */
				/* compute expected IDAT crc */
				long offset = CHUNK_LEN_SIZE + CHUNK_TYPE_SIZE + png_height*(png_width*4 + 1);
				/* move the file pointer */
				if(fseek(f, offset, SEEK_CUR) != 0){
					/* for handling error */
					printf("Error: can't move file pointer\n");
					return -1;
				}
				expected_crc = crc(f, CHUNK_CRC_SIZE);
				/* read IDAT crc */
				elementsRead = fread(crc_p, CHUNK_CRC_SIZE, 1, f);
				if(elementsRead != 1){
					/* error handling */
					printf("Error: can't read the CRC of %s\n", argv[1]);
				}
				if(expected_crc != *crc_p){
					printf("IDAT chunk CRC error: computed %d, expected %d\n", *crc_p, expected_crc);
				}

			/* IEND crc check */
				/* compute expected IEND crc */
				offset = CHUNK_LEN_SIZE + CHUNK_TYPE_SIZE;
				/* move the file pointer */
				if(fseek(f, offset, SEEK_CUR) != 0){
					/* for handling error */
					printf("Error: can't move file pointer\n");
					return -1;
				}
				expected_crc = crc(f, CHUNK_CRC_SIZE);
				/* read IEND crc */
				elementsRead = fread(crc_p, CHUNK_CRC_SIZE, 1, f);
				if(elementsRead != 1){
					/* error handling */
					printf("Error: can't read the CRC of %s\n", argv[1]);
				}
				if(expected_crc != *crc_p){
					printf("IEND chunk CRC error: computed %d, expected %d\n", *crc_p, expected_crc);
				}
		free(crc_p);
	} else if(png_state == 0){
		printf("%s: Not a PNG file\n", argv [1]);
	} else{
		/* error handling */
		printf("Error: can't read the header of %s", argv[1]);
	}
	fclose(f);
	free(header);
	return 0;
}

/* Checks whether 3-byte identifier matches the one for PNG images */
int is_png(U8 *buf, size_t n){
	/* Check if header size is correct and if buf is empty */
	if(n != 8 || buf == NULL){
		return -1;
	}

	/* Check if the file is a PNG */
	if(buf[1] == 0x50 && buf[2] == 0x4E && buf[3] == 0x47){
		return 1;
	} else{
		return 0;
	}
}

/* obtain the height of the png file */
int get_png_height(struct data_IHDR *buf){
	/* check if pointer is null */
	if(buf == NULL){
		printf("Error with get_png_height: IHDR pointer is NULL.\n");
		return -1;
	}
	return (int)bigEnd_to_littleEnd(buf-> height);
}

/* obtain the width of the png file */
int get_png_width (struct data_IHDR *buf){
	/* check if the pointer is null */
    if(buf == NULL){
		printf("Error with get_png_height: IHDR pointer is NULL.\n");
		return -1;
	}
	return (int)bigEnd_to_littleEnd(buf->width);
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
	return 0;
}

/* convert big endian to little endian */
U32 bigEnd_to_littleEnd(U32 bigEnd_val){
	U32 littleEnd_val = ((bigEnd_val >>  24) & 0xFF) | 
						((bigEnd_val >> 8) & 0xFF00) | 
						((bigEnd_val << 8) & 0xFF0000) |
						((bigEnd_val << 24) & 0xFF000000);

	return littleEnd_val;	
}