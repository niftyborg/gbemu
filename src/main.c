#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

#include <cJSON.h>

#define SM83_DIR "./sm83/v1/"

int cmp_str (const void *a, const void *b) {
    return strcmp(*(char**)a, *(char**)b);
}

int test_file_list(char **filenames, size_t *filecount) {
    int error = 0;
    struct dirent *dir = {0};
    DIR *d = {0};
    assert (filenames != NULL && "Failed to alloc filenames");

    d = opendir(SM83_DIR); // Open the current directory
    int n_files = 0;
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if(dir->d_name[0] == '.') continue; // exclude . and ..
            filenames[n_files] = strdup(dir->d_name);
            n_files += 1;
        }
        *filecount = n_files;
    } else {
        printf("Failed to read %s\n", SM83_DIR);
        error = 1;
    }
    if (d) closedir(d);
    return error;
}

int main(void) {

    char* buffer = malloc(1 << 20);
    if (buffer == NULL)
        return EXIT_FAILURE;

    char **filenames = calloc(1024, sizeof(char*));
    size_t filenames_count = 0;
    test_file_list (filenames, &filenames_count);
    qsort(filenames, filenames_count, sizeof(char*), cmp_str);

    printf("Test Files\n");
    for (size_t i = 0; i < filenames_count; i++)
        printf("%zu: %s\n", i, filenames[i]);

    // for(int file_idx = 0; file_idx < n_files; file_idx++){
    //     char* file_name = strcat("./sm83/v1", )
    // }

    // TODO: Test File not in repo
    // FILE *fp = fopen("asdf.c8", "rb");
    // int bytesRead = fread(buffer, 1, 1<<20, fp);
    // printf("%x%x\n", buffer[0], buffer[1]);
    if (buffer) free(buffer);
    if (filenames) {
        for (size_t i = 0; i < filenames_count; i++) free(filenames[i]);
        free (filenames);
    }
    return 0;
}
