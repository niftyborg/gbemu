#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include "cJSON.h"

int main(void) {

    char* buffer = malloc(1 << 20);
    struct dirent *dir;
    DIR *d;
    d = opendir("./sm83/v1/"); // Open the current directory
    char* filenames[1024];
    int n_files = 0;
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if(dir->d_name[0] == '.') continue; // exclude . and ..
            filenames[n_files] = strdup(dir->d_name);
            n_files += 1;
        }
        closedir(d);
    } else {
        printf("No test JSONs found!");
    }

    // for(int file_idx = 0; file_idx < n_files; file_idx++){
    //     char* file_name = strcat("./sm83/v1", )
    // }

    // TODO: Test File not in repo
    // FILE *fp = fopen("asdf.c8", "rb");
    // int bytesRead = fread(buffer, 1, 1<<20, fp);
    // printf("%x%x\n", buffer[0], buffer[1]);
    return 0;
}
