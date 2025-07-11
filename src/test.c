#include "test.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>

#include "util.h"

static int test_file_list(char **filenames, size_t *filecount);
static void run_test_group_ (struct sm83_test *tests, size_t count,
                     size_t *success_count);
static void run_test_group (char *file_name, struct sm83_test *tests,
                            size_t tests_len);

int cmp_str (const void *a, const void *b) {
    return strcmp(*(char**)a, *(char**)b);
}

void
run_tests (void)
{
    char **file_names = calloc(1024, sizeof(char*));
    size_t file_names_count = 0;
    test_file_list (file_names, &file_names_count);
    qsort(file_names, file_names_count, sizeof(char*), cmp_str);

    struct sm83_test *tests = NULL;
    size_t tests_len = 0;
    char* file_path;
    for (size_t i = 0; i < file_names_count && i < 0x7F; i++){
        file_path = malloc(strlen(SM83_DIR) + strlen(file_names[i]) + 1);
        assert (file_path && "Failed to allocate space for full path name");

        sprintf (file_path, "%s%s", SM83_DIR, file_names[i]);
        parse_file (file_path, &tests, &tests_len);
        assert (tests != NULL && tests_len > 0 && "Failed to parse JSON");
        run_test_group (file_names[i], tests, tests_len);

        free (file_path);
    }
}

void
run_test (char *test_name)
{
    assert (test_name);

    char **file_names = calloc(1024, sizeof(char*));
    size_t file_names_count = 0;
    test_file_list (file_names, &file_names_count);
    qsort(file_names, file_names_count, sizeof(char*), cmp_str);

    struct sm83_test *tests = NULL;
    size_t tests_len = 0;
    char* file_path;
    char* file_name;
    for (size_t i = 0; i < file_names_count && i < 0x7F; i++){
        file_name = file_names[i];
        if (file_name != NULL && strstr (file_name, test_name) == NULL)
            continue;

        file_path = malloc(strlen(SM83_DIR) + strlen (file_name) + 1);
        assert (file_path && "Failed to allocate space for full path name");

        sprintf (file_path, "%s%s", SM83_DIR, file_names[i]);
        parse_file(file_path, &tests, &tests_len);
        assert (tests != NULL && tests_len > 0 && "Failed to parse JSON");
        run_test_group (file_names[i], tests, tests_len);

        free (file_path);
    }
}

void
run_test_group (char *file_name, struct sm83_test *tests, size_t tests_len)
{
    size_t success_count;
    success_count = 0;
    printf ("==============================\n");
    printf ("FILE TEST: %s\n", file_name);
    printf ("------------------------------\n");
    run_test_group_ (tests, tests_len, &success_count);
    printf ("\t Result: %zu/%zu\n", success_count, tests_len);
    printf ("\n");
}

static int
test_file_list(char **filenames, size_t *filecount)
{
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

static void
run_test_group_ (struct sm83_test *tests, size_t count, size_t *success_count)
{
    size_t sc = 0;
    for (size_t i = 0; i < count; i++) {
        struct sm83_test current_test = tests[i];
        int pass = run_sm83_test (current_test);
        if (pass == 0) {
            sc++;
            continue; // only show tests that failed
        }
        /* printf("\t[%s]: %s\n", current_test.name, pass ? "FAILURE" : "SUCCESS"); */
    }
    *success_count = sc;
}
