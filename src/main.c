#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include <cJSON.h>

#include "util.h"
#include "jtest.h"
#include "test.h"
#include "sim.h"

enum FLAGS {
FALLTESTS = 0 << 0,
FSIM      = 1 << 0,
FTEST     = 1 << 2,
};

int
main(int argc, char **argv)
{
    int opt = 0;
    enum FLAGS f = {0};
    char *test_name = {0};

    while ((opt = getopt(argc, argv, "st:")) != -1) {
        switch (opt) {
            case 's':
                f |= FSIM;
                break;
            case 't':
                test_name = strdup (optarg);
                f |= FTEST;
                break;
            default:
                error ("usage: %s [-t <test_name>] [-s]", argv[0]);
                error ("\tRuns all tests cases by default if no flags are provided");
                exit (EXIT_FAILURE);
        }
    }

    if (f & FTEST)
        run_test (test_name);
    else if (f & FSIM)
        run_sim ();
    else
        run_tests ();

    return EXIT_SUCCESS;
}
