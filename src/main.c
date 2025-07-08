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
#include <raylib.h>

#include "util.h"
#include "jtest.h"

#define SM83_DIR "./sm83/v1/"
#define WINDOW_TITLE "Game Boy Emulator"
#define TARGET_FPS 10
#define WIN_SCALE 5

struct dim {
    size_t h;
    size_t w;
};

// BSS reserve
#define FILE_BUF_LEN 1024 * 1024
char file_buf[FILE_BUF_LEN + 1] = {0};

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

int read_file(char *file_path, struct string *s) {
    int err = 0;
    int fd = open(file_path, O_RDONLY);
    if (fd < 0) {
        error ("Failed to read %s due to %s", file_path, strerror(errno));
        err = 1;
        goto error;
    }

    ssize_t n = 0;
    while ((n = read(fd, file_buf, FILE_BUF_LEN)) > 0) {
        string_appendn(s, file_buf, n);
    }

error:
    if (fd >= 0) close(fd);
    return err;
}

int parse_file(char *file_path, struct sm83_test **sm83_tests, size_t *length) {
    int err = 0;
    // get raw data
    struct string file_data = {0};
    (void)read_file(file_path, &file_data);
    if (file_data.str == NULL) {
        error("Failed to read file %s", file_path);
        err = 1;
        goto error;
    }
    // parse json
    cJSON *tests = cJSON_Parse((char*) file_data.str);
    if (tests == NULL) {
        const char *json_err = cJSON_GetErrorPtr();
        assert (json_err != NULL && "Failed to know JSON error");
        error("Failed to parse file %s with cJSON: %s", file_path, json_err);
        err = 2;
        goto error;
    }
    ssize_t test_len = cJSON_GetArraySize(tests);
    if (test_len < 0) {
        error("Expected test array with non-negative size for file %s",
              file_path);
        err = 3;
        goto error;
    }
    // tests
    *length = test_len;
    struct sm83_test *sm83ts = calloc(test_len + 1, sizeof(struct sm83_test));
    if (sm83ts == NULL) {
        error("Failed to allocate sm83 tests");
        err = 4;
        goto error;
    }
    size_t test_idx = 0;

    // TODO: definitely missing some registers, will check later
    #define KNAMES_LEN 12
    static char *knames[KNAMES_LEN] =
        { "pc", "sp", "a", "b", "c", "d", "e", "f", "h", "l", "ime", "ei" };

    // TODO: clean up allocations on errors
    struct gbs_kv kv;
    struct ram_state rstate;
    struct sm83_test st;
    cJSON *test = NULL;
    cJSON *rpair = NULL;
    cJSON *rp_t = NULL;
    cJSON_ArrayForEach(test, tests) {
        kv = (struct gbs_kv) {0};
        rstate = (struct ram_state) {0};
        st = (struct sm83_test) {0};

        cJSON *name = cJSON_GetObjectItemCaseSensitive(test, "name");
        if (!cJSON_IsString(name) || name->valuestring == NULL) {
            error("Missing 'name', Malformed test file %s", file_path);
            err = 4;
            goto error;
        }
        st.name = strdup(name->valuestring);
        // initials
        cJSON *initial = cJSON_GetObjectItemCaseSensitive(test, "initial");
        if (!cJSON_IsObject(initial)) {
            error("Missing 'initial', malformed test file %s", file_path);
            err = 4;
            goto error;
        }
        // register, etc states
        for (int i = 0; i < KNAMES_LEN; i++) {
            cJSON *t = cJSON_GetObjectItemCaseSensitive(initial, knames[i]);
            if (t == NULL || !cJSON_IsNumber(t)) continue;
            kv.k = strdup(knames[i]);
            kv.v = t->valueint;
            (void)kvs_append(&st.initial.kvs, &kv);
        }
        // ram state
        cJSON *initial_ram = cJSON_GetObjectItemCaseSensitive(initial, "ram");
        if (!cJSON_IsArray(initial_ram)) {
            error("Missing 'final', malformed test file %s", file_path);
            err = 4;
            goto error;
        }
        cJSON_ArrayForEach(rpair, initial_ram) {
            // TODO: validate that the pair is in fact a pair with array size == 2
            rp_t = cJSON_GetArrayItem(rpair, 0);
            assert (rp_t != NULL && cJSON_IsNumber(rp_t)); // TODO: proper err chk
            rstate.pos = rp_t->valueint;
            rp_t = cJSON_GetArrayItem(rpair, 1);
            assert (rp_t != NULL && cJSON_IsNumber(rp_t)); // TODO: proper err chk
            rstate.val = rp_t->valueint;
            (void)rams_append(&st.initial.rams, &rstate);
        }

        // finals
        cJSON *final = cJSON_GetObjectItemCaseSensitive(test, "final");
        if (!cJSON_IsObject(final)) {
            error("Missing 'final', malformed test file %s", file_path);
            err = 4;
            goto error;
        }
        // register, etc states
        for (int i = 0; i < KNAMES_LEN; i++) {
            cJSON *t = cJSON_GetObjectItemCaseSensitive(final, knames[i]);
            if (t == NULL || !cJSON_IsNumber(t)) continue;
            kv.k = strdup(knames[i]);
            kv.v = t->valueint;
            kvs_append(&st.final.kvs, &kv);
        }
        // ram state
        cJSON *final_ram = cJSON_GetObjectItemCaseSensitive(initial, "ram");
        if (!cJSON_IsArray(final_ram)) {
            error("Missing 'final', malformed test file %s", file_path);
            err = 4;
            goto error;
        }
        cJSON_ArrayForEach(rpair, final_ram) {
            // TODO: validate that the pair is in fact a pair with array size == 2
            rp_t = cJSON_GetArrayItem(rpair, 0);
            assert (rp_t != NULL && cJSON_IsNumber(rp_t)); // TODO: proper err chk
            rstate.pos = rp_t->valueint;
            rp_t = cJSON_GetArrayItem(rpair, 1);
            assert (rp_t != NULL && cJSON_IsNumber(rp_t)); // TODO: proper err chk
            rstate.val = rp_t->valueint;
            (void)rams_append(&st.final.rams, &rstate);
        }

        sm83ts[test_idx++] = st;
    }
    *sm83_tests = sm83ts;
error:
    // TODO: cleanup all allocations
    if (file_data.str != NULL) free (file_data.str);
    return err;
}

void run_tests(struct sm83_test *tests, size_t count, void *gbm_state) {
    printf("SM83 TESTS: RUNNING\n");
    size_t success_count = 0;
    for (int i = 0; i < count; i++) {
        struct sm83_test current_test = tests[i];
        int pass = run_sm83_test(current_test);
        if (pass == 0) {
            success_count++;
            continue; // only show tests that failed
        }
        // printf("\t[%s]: %s\n", current_test.name, pass ? "FAILURE" : "SUCCESS");
    }
    printf("SM83 RESULTS: %zu/%zu\n", success_count, count);
}

void dump_rom(struct string *rom_data) {
    for (size_t i = 0; i < rom_data->len; i++) {
        if (isprint(rom_data->str[i]))
            printf("%c", rom_data->str[i]);
        else
            printf(".");
    }
    printf("\n");
}

int main(void) {
#ifdef RUN_TESTS
    char **filenames = calloc(1024, sizeof(char*));
    size_t filenames_count = 0;
    test_file_list (filenames, &filenames_count);
    qsort(filenames, filenames_count, sizeof(char*), cmp_str);

    printf("Test Files\n");
    for (size_t i = 0; i < filenames_count && i < 10; i++)
        printf("%zu: %s\n", i, filenames[i]);

    struct sm83_test *tests = NULL;
    size_t tests_len = 0;
    for (size_t i = 0; i < filenames_count && i < 0x7F; i++){
        char* file = malloc(strlen(SM83_DIR) + strlen(filenames[i]) + 1);
        strcpy(file, SM83_DIR);
        strcat(file, filenames[i]);
        parse_file(file, &tests, &tests_len);
        assert (tests != NULL);
        printf("running test %s\n", filenames[i]);
        // sm83_test_dump(tests, tests_len);

        run_tests(tests, tests_len, NULL);

    }
#else
    // Game Boy screen: 160px across by 144px
    struct dim vdim = { .w = 160, .h = 144 };
    // physical screen
    struct dim pdim = { .w = vdim.w * WIN_SCALE * 2, .h = vdim.h * WIN_SCALE };

    struct string rom_data = {0};
    assert (read_file("./roms/tetris.gb", &rom_data) == 0
            && "Failed to load tetris rom");
    /* dump_rom(&rom_data); */

    InitWindow(pdim.w, pdim.h, WINDOW_TITLE);
    SetTargetFPS(TARGET_FPS);
    while (!WindowShouldClose()) {
        /* update_input (&ctx); */
        /* for (int i = 0; i < 10; i++) ops_dispatch(&ctx); */

        BeginDrawing();
        ClearBackground((Color) { .r = 0, .g = 0, .b = 0, .a = 255 });
        // REGISTER VALUES
        Vector2 text_pos = { .x = pdim.w/2, .y = 0 };
        Vector2 reg_spacing = { .x = 60, .y = 20 };
        for (size_t i = 0; i < REG_LEN; i++) {
            size_t x_off = i % 6 * reg_spacing.x;
            if (i % 6 == 0) text_pos.y += reg_spacing.y;
            size_t font_size = reg_spacing.x;
            DrawText(TextFormat("%02d: 0", i), text_pos.x + x_off, text_pos.y, 14, WHITE);
        }
        // INSTRUCTIONS
        size_t curr_pos = 0;
        text_pos = (Vector2) { .x = pdim.w/2, .y = pdim.h/2 };
        Vector2 op_spacing = { .x = 60, .y = 20 };
        for (size_t i = 0; curr_pos < rom_data.len && i < 20; curr_pos++, i++) {
            size_t x_off = i % 6 * reg_spacing.x;
            if (i % 6 == 0) text_pos.y += reg_spacing.y;
            size_t font_size = reg_spacing.x;
            DrawText(TextFormat("%02X", rom_data.str[curr_pos]), text_pos.x + x_off, text_pos.y, 14, WHITE);
        }
        EndDrawing();
    }
    CloseWindow();
#endif // ONLY_TESTS

    return EXIT_SUCCESS;
}
