#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

void recursiveSearch(char* pathname, int* fileCount);
void getSignature(const unsigned char* png_buffer, unsigned long long* signature);

int main(int argc, char *argv[]) {

    int fileCount = 0;

    if (argc == 1) {
        printf("findpng: No PNG file found\n");
        return 0;
    }

    /* Append "/" onto relative path to work correctly */
    if (!strcmp(argv[1], ".") || !strcmp(argv[1], "..")) {
        char* relativepath = malloc(strlen(argv[1])+2);
        strcpy(relativepath, argv[1]);
        strcat(relativepath, "/");
        argv[1] = relativepath;

        recursiveSearch(argv[1], &fileCount);
        free(argv[1]);
        return 0;
    }
    
    /* If path is not a directory */
    struct stat path_stat;
    stat(argv[1], &path_stat);
    if (S_ISREG(path_stat.st_mode)) {
        unsigned long long signature;
    	DIR* path = opendir(argv[1]);
    	
    	FILE* png = fopen(argv[1], "r");
        fseek(png, 0, SEEK_END);
        long png_size = ftell(png);
        fseek(png, 0, SEEK_SET);
        unsigned char* png_buffer = malloc(png_size);
        fread(png_buffer, 1, png_size, png);
        getSignature(png_buffer, &signature);

        if (signature == 0x89504e470d0a1a0a) {
            printf("%s\n", argv[1]);
        }
        else {
            printf("findpng: No PNG file found\n");
        }

        fclose(png);
        free(png_buffer);
        return 0;
    }

    recursiveSearch(argv[1], &fileCount);
    return 0;
}

void recursiveSearch(char* pathname, int* fileCount) {

    DIR* path = opendir(pathname);    /* Points to a directoiry stream to be read from */
    struct dirent* directory = NULL;  /* Will store the read path for traversal */

    /* Directory does not exist */
    if (ENOENT == errno) {
        printf("findpng: No PNG file found\n");
        return;
    }

    /* While still files left in directory, continue loop */
    /* Everytime readdir is called we check next file at next position */
    while((directory = readdir(path)) != NULL) {

        /* Check type */
        if (directory->d_type == DT_REG) { /*Regular*/
            unsigned long long signature;

            /* Concat root path and file name to get path of png */
            char* fullpath = malloc(strlen(pathname)+strlen(directory->d_name)+1);
            strcpy(fullpath, pathname);
            strcat(fullpath, directory->d_name);

            /* Open file and get signature */
            FILE* png = fopen(fullpath, "r");
            fseek(png, 0, SEEK_END);
            long png_size = ftell(png);
            fseek(png, 0, SEEK_SET);
            unsigned char* png_buffer = malloc(png_size);
            fread(png_buffer, 1, png_size, png);
            getSignature(png_buffer, &signature);

            if (signature == 0x89504e470d0a1a0a) {
                ++*fileCount;
                printf("%s\n", fullpath);
            }

            fclose(png);
            free(fullpath);
            free(png_buffer);
        }
        /* If hit new folder, search it too */
        else if (directory->d_type == DT_DIR) { /*Directory*/

            /* Special case for first two recursions (we don't want . and .. paths) */
            if (!strcmp(directory->d_name, ".") || !strcmp(directory->d_name, "..")) {
                continue;
            }

            char fullpath[strlen(pathname)+strlen(directory->d_name)+2];
            strcpy(fullpath, pathname);
            strcat(fullpath, directory->d_name);
            strcat(fullpath, "/");

            recursiveSearch(fullpath, fileCount);
        }
    }

    if (*fileCount == 0) {
        printf("findpng: No PNG file found\n");
    }

    closedir(path);
}

void getSignature(const unsigned char* png_buffer, unsigned long long* signature) {
    unsigned long long signature_temp1 = (png_buffer[0] << 24) + (png_buffer[1] << 16) + (png_buffer[2] << 8) + png_buffer[3];
    unsigned long long signature_temp2 = (png_buffer[4] << 24) + (png_buffer[5] << 16) + (png_buffer[6] << 8) + png_buffer[7];
    *signature = (signature_temp1 << 32) + signature_temp2;
}
