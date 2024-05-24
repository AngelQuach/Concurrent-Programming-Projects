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
void SearchDIR(char *dir_path){
     DIR *p_dir;
    struct dirent *p_dirent;
    char str[64];
    struct stat buf;

    
    if (lstat(dir_path, &buf) < 0) {
            perror("lstat error");
        }   


    if ((p_dir = opendir(dir_path)) == NULL) {
        sprintf(str, "opendir(%s)", dir_path);
        perror(str);
        exit(2);
    }

    while ((p_dirent = readdir(p_dir)) != NULL) {
        char path[1024];
        char *str_path = p_dirent->d_name;  /* relative path name! */
        snprintf(path, sizeof(path), "%s/%s", dir_path, str_path);

        if (str_path == NULL) {
            fprintf(stderr,"Null pointer found!"); 
            exit(3);
        } else if (S_ISDIR(buf.st_mode)){
            SearchDIR(path);      
        } else if (S_ISREG(buf.st_mode)){
            /*check if the file is a png file*/
            FILE *f = fopen(path, "rb");

            if(f == NULL){
		        printf("Unable to open file. %s is a invalid name?\n", dir_path);
		        exit(1);
	        }

            /* We want to read 8-byte identifier from the header */
            U8 *header = malloc(8);
	        size_t elementsRead = fread(header, sizeof(U8), 8, f);
	        if(elementsRead != 8){
		        printf("Error reading the file. Expected elements: 8. Actual elements: %ld\n", elementsRead);
		        fclose(f);
		        free(header);
		        exit(1);
	        }
            int png_state = is_png(header, 8);
            if(png_state == 1){
                printf("%s\n", str_path);
            }
        }

    }
    if ( closedir(p_dir) != 0 ) {
        perror("closedir");
        exit(3);
    }

}


int main(int argc, char *argv[]) 
{
//   DIR *p_dir;
//     struct dirent *p_dirent;
//     char str[64];
//     struct stat buf;

//     int i;
//     if (lstat(argv[i], &buf) < 0) {
//             perror("lstat error");
//         }   

     if (argc == 1) {
         fprintf(stderr, "Usage: %s <directory name>\n", argv[0]);
         exit(1);
     }

     SearchDIR(argv[1]);

//     if ((p_dir = opendir(argv[1])) == NULL) {
//         sprintf(str, "opendir(%s)", argv[1]);
//         perror(str);
//         exit(2);
//     }

//     while ((p_dirent = readdir(p_dir)) != NULL) {
//         char *str_path = p_dirent->d_name;  /* relative path name! */

//         if (str_path == NULL) {
//             fprintf(stderr,"Null pointer found!"); 
//             exit(3);
//         } else if (S_ISDIR(buf.st_mode)){
//             SearchDIR(argv[1]);      
//         } else if (S_ISREG(buf.st_mode)){
//             /*check if the file is a png file*/
//             FILE *f = fopen(argv[1], "rb");

//             if(f == NULL){
// 		        printf("Unable to open file. %s is a invalid name?\n", argv[1]);
// 		        return -1;
// 	        }

//             /* We want to read 8-byte identifier from the header */
//             U8 *header = malloc(8);
// 	        size_t elementsRead = fread(header, sizeof(U8), 8, f);
// 	        if(elementsRead != 8){
// 		        printf("Error reading the file. Expected elements: 8. Actual elements: %ld\n", elementsRead);
// 		        fclose(f);
// 		        free(header);
// 		        return -1;
// 	        }
//             int png_state = is_png(header, 8);
//             if(png_state == 1){
//                 printf("%s\n", str_path);
//             }
//         }

//     }
//     if ( closedir(p_dir) != 0 ) {
//         perror("closedir");
//         exit(3);
//     } 

    return 0;
}