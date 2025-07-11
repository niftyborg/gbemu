#ifndef UTIL_H_
#define UTIL_H_
#include <limits.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define SM83_DIR "./sm83/v1/"

#define FILE_BUF_LEN 1024 * 1024

void non_fatal_ (char *file, int line, char *level, char *msg, ...);

#define info(...)                                               \
    do {                                                        \
        non_fatal_ (__FILE__, __LINE__, "INFO", __VA_ARGS__);   \
    } while (0)

#define error(...)                                              \
    do {                                                        \
        non_fatal_ (__FILE__, __LINE__, "ERROR", __VA_ARGS__);   \
    } while (0)

#define unreachable()                                                   \
    do {                                                                \
        fprintf (stderr, "%s:%d: %s\n", __FILE__, __LINE__, "UNREACHABLE"); \
        abort ();                                                       \
    } while (0)

struct vec {
    unsigned char *items;
    size_t len;
    size_t cap;
};

struct string {
    unsigned char *str;
    size_t len;
    size_t cap;
};

// Vec framework, implement functions in terms of your type
void vec_reset       (struct vec *vec);
int vec_append       (struct vec *vec, size_t item_size, void *val);
int vec_pop          (struct vec *vec, size_t item_size, void *val);
int vec_append_many  (struct vec *vec, size_t item_size, void *vals, size_t count);

// String, specialization of vec
int string_appendn      (struct string *s, char *cs, size_t n);
int string_append       (struct string *s, char *cs);
int string_append_char  (struct string *s, char c);
void string_print_dbg   (struct string *s);
void string_reset       (struct string *s);

// IO
int read_file (char *file_path, struct string *s);
void dump_rom (struct string *rom_data);
#endif // UTIL_H_

#ifdef UTIL_IMPLEMENTATION
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

void
non_fatal_ (char *file, int line, char *level, char *msg, ...)
{
    assert (msg);
    va_list ap;
    va_start (ap, msg);
    fprintf (stderr, "%s:%d: %s: ", file, line, level);
    vfprintf (stderr, msg, ap);
    fprintf (stderr, "\n");
    va_end (ap);
}

inline void
vec_reset (struct vec *vec)
{
    assert (vec);
    vec->len = 0;
}

inline int
vec_append (struct vec *vec, size_t item_size, void *val)
{
    assert (vec);
    assert (val);
    assert (item_size > 0);
    if (vec->items == NULL) {
        vec->items = calloc (vec->cap, item_size);
        if (vec->items == NULL) return 1;
    }
    if (vec->len == vec->cap) {
        size_t new_cap = vec->cap == 0 ? 8 : vec->cap * 2;
        unsigned char *tmp = calloc (new_cap, item_size);
        if (tmp == NULL) return 1;
        memcpy (tmp, vec->items, vec->len * item_size);
        free (vec->items);
        vec->items = tmp;
        vec->cap = new_cap;
    }
    if (vec->len < vec->cap) {
        memcpy (vec->items + (vec->len * item_size), val, item_size);
        vec->len++;
    }
    else {
        assert (0 && "Somehow len is equal to or greater than capacity, undefined");
    }
    return 0;
}

inline int
vec_pop (struct vec *vec, size_t item_size, void *val)
{
    assert (vec && vec->cap > 0 && val && item_size > 0);
    assert (vec->len <= vec->cap && "Somehow len exceeds cap, undefined");
    memcpy (vec->items + ((vec->len - 1) * item_size), val, item_size);
    vec->len--;
    return 0;
}

inline int
vec_append_many (struct vec *vec, size_t item_size, void *vals, size_t count)
{
    assert (vec && vals);
    assert (item_size > 0);
    size_t old_len = vec->len;
    char *vs = vals;
    int commit = 1;
    for (size_t i = 0; i < count; i++) {
        char *v = vs + (i * item_size);
        if (vec_append (vec, item_size, v)) {
            commit = 0;
            break;
        }
    }
    if (!commit) {
        vec->len = old_len;
        return 1;
    }
    return 0;
}

int
string_appendn (struct string *s, char *cs, size_t n)
{
    assert (s && cs);
    return vec_append_many ((struct vec*)&s->str, sizeof (char),
                            cs, n);
}

int
string_append (struct string *s, char *cs)
{
    assert (s && cs);
    return vec_append_many ((struct vec*)&s->str, sizeof (char),
                            cs, strlen (cs));
}

int
string_append_char (struct string *s, char c)
{
    return vec_append ((struct vec*)&s->str, sizeof (c), &c);
}

void
string_print_dbg (struct string *s)
{
    for (size_t i = 0; i < s->len; i++) {
        if (iscntrl (s->str[i]))
            printf (".");
        else
            printf ("%c", s->str[i]);
    }
}

void
string_reset (struct string *s)
{
    /* memset (s->str, 0, s->cap); */
    vec_reset ((struct vec*) &s->str);
}

// WARN: not thread safe
int read_file(char *file_path, struct string *s) {
    static char file_buf[FILE_BUF_LEN + 1] = {0};

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

void dump_rom(struct string *rom_data) {
    for (size_t i = 0; i < rom_data->len; i++) {
        if (isprint(rom_data->str[i]))
            printf("%c", rom_data->str[i]);
        else
            printf(".");
    }
    printf("\n");
}

#endif // COMMON_IMPLEMENTATION
