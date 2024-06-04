#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>   /* for printf().  man 3 printf */
#include <stdlib.h>  /* for exit().    man 3 exit   */
#include <string.h>  /* for strcat().  man strcat   */
#include <fcntl.h>
#include "lab_png.h"
#include "crc.h"



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

/* Searches the subdirectory */
void SearchDIR(char *dir_path, int *count){
     DIR *p_dir;
    struct dirent *p_dirent;
    char str[64];
    struct stat buf;
   


    if ((p_dir = opendir(dir_path)) == NULL) {
        sprintf(str, "opendir(%s)", dir_path);
        perror(str);
        exit(2);
    }

    while ((p_dirent = readdir(p_dir)) != NULL) {
        char path[1024];
        char *str_path = p_dirent->d_name;  /* relative path name! */
        snprintf(path, sizeof(path), "%s/%s", dir_path, str_path);

        if (lstat(path, &buf) < 0) {
            perror("lstat error");
            continue;
        }

        if (str_path == NULL) {
            fprintf(stderr,"Null pointer found!"); 
            exit(3);
        } else if (S_ISDIR(buf.st_mode)){
            if (strcmp(p_dirent->d_name, ".") != 0 && strcmp(p_dirent->d_name, "..") != 0){
                SearchDIR(path, count);
            }      
        } else if (S_ISREG(buf.st_mode)){
            /*check if the file is a png file*/
            FILE *f = fopen(path, "rb");

            if(f == NULL){
		        printf("Unable to open file. %s is a invalid name?\n", path);
		        exit(2);
	        }

            /* We want to read 8-byte identifier from the header */
            U8 *header = malloc(8);
	        size_t elementsRead = fread(header, sizeof(U8), 8, f);
	        if(elementsRead != 8){
		        /*printf("Error reading the file. Expected elements: 8. Actual elements: %ld\n", elementsRead);*/
		        fclose(f);
		        free(header);
		        continue;
	        }
            int png_state = is_png(header, 8);
            if(png_state == 1){
                printf("%s\n", path);
                *(count) = *(count) + 1;
            }
            fclose(f);
            free(header);
        }

    }
    if ( closedir(p_dir) != 0 ) {
        perror("closedir");
        exit(3);
    }

}

int main(int argc, char *argv[]) 
{   
    int count = 0;
    if (argc == 1) {
         fprintf(stderr, "Usage: %s <directory name>\n", argv[0]);
         exit(1);
    }
    SearchDIR(argv[1], &count);
    if(count == 0){
        printf("No PNG file found\n");
    }
    return 0;
}