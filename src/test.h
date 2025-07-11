#ifndef TEST_H_
#define TEST_H_

#include <stdint.h>
#include <stddef.h>
#include "jtest.h"

void run_tests (void);
void run_test (char *test_name);
/* void run_test_group (char *file_name, struct sm83_test *tests, size_t tests_len); */
/* int test_file_list(char **filenames, size_t *filecount); */
#endif // TEST_H_
