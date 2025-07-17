#ifndef TEST_H_
#define TEST_H_

#include <stdint.h>
#include <stddef.h>
#include "jtest.h"

extern int show_completed; // whether to show passing test groups
extern int show_individual; // how many of the individual tests to show

void run_tests (void);
void run_test (char *test_name);
#endif // TEST_H_
