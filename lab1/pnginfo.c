/**
 * @file: ECE252/lab1/pnginfo.c
 * @brief: Checks if the file is a PNG and output dimensions
 * @author: Angel Quach, Taylor Yin
 * @date: 2024/05/21
 */

#include <sys/type.h>
#include <sys/stat.h>
#include <studio.h>
#include <stdlib.h>
#include <unistd.h>
#include "lab_png.h"

int main(int argc, char *argv[]){
		/* check if input is of correct format */
		if(argc <= 1){
				printf("Format: pnginfo <filename>");
				return -1;
		}

		/* open the file specified */
		FILE* f = fopen(argv[1], "r");
		/* check if the file exists */
		if(f == NULL){
				printf("Unable to open file. %s is a invalid name?\n", argv[1]);
				fclose(f);
				return -1;
		}

		/* We want to read 8-byte identifier from the header */
		U8 *header;
		int bytesRead = read(f, header, 8);
		if(bytesRead != 8){
				printf("Error reading the file. Expected bytes: 8. Actual bytes: %zd\n", bytesRead);
				fclose(f);
				return -1;
		}
		int png_state = is_png(header, 8);
		if(png_state == 1){
				printf("%s: Successfully Read", argv [1]);
				return 0;
		} else if(png_state == 0){
				printf("%s: Not a PNG file", argv [1]);
				return 0;
		} else{
				printf("%s: CRC error");
		}
}

/* Checks whether 3-byte identifier matches the one for PNG images */
int is_png(U8 *buf, size_t n){
		/* Check if header size is correct */
		if(n != 8){
				return -1;
		}

		/* Check if buf is empty 8 */
		if(buf == NULL){
				printf("Error: Header pointer is NULL.\n");
				return -1;
		}

		/* Check if the file is a PNG */
		if(buf[1] == 0x50 && buf[2] == 0x4E && buf[3] == 0x47){
				return 1;
		} else{
				return 0;
		}
}
