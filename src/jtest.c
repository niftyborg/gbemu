#include <assert.h>
#include <stdio.h>

#include "jtest.h"
#include "util.h"

void kvs_reset(struct kvs *kvs) {
    assert (kvs);
    vec_reset((struct vec*) kvs->fields);
}

int kvs_append(struct kvs *kvs, struct gbs_kv *val) {
    assert (kvs);
    return vec_append((struct vec*) &kvs->fields, sizeof(struct gbs_kv), val);
}

int kvs_pop(struct kvs *kvs, struct gbs_kv *val) {
    assert (kvs && kvs->fields);
    return vec_pop((struct vec*) &kvs->fields, sizeof(struct gbs_kv), val);
}

void rams_reset(struct rams *rams) {
    assert (rams);
    vec_reset((struct vec*) rams->states);
}

int rams_append(struct rams *rams, struct ram_state *val) {
    assert (rams);
    return vec_append((struct vec*) &rams->states, sizeof(struct ram_state), val);
}

int rams_pop(struct rams *rams, struct ram_state *val) {
    assert (rams && rams->states);
    return vec_pop((struct vec*) &rams->states, sizeof(struct ram_state), val);
}

void sm83_test_dump(struct sm83_test *tests, size_t tests_len) {
    for (size_t i = 0; i < tests_len && i < 10; i++) {
        printf("Name: %s\n", tests[i].name);
        printf("\tInitial KVs:");
        for (size_t j = 0; j < tests[i].initial.kvs.len; j++) {
            struct gbs_kv kv = tests[i].initial.kvs.fields[j];
            printf("[%s, %zu]", kv.k, kv.v);
        }
        printf("\n");
        printf("\tInitial RAM States:");
        for (size_t j = 0; j < tests[i].initial.rams.len; j++) {
            struct ram_state rs = tests[i].initial.rams.states[j];
            printf("[%zu, %zu]", rs.pos, rs.val);
        }
        printf("\n");
        printf("\tFinal KVs:");
        for (size_t j = 0; j < tests[i].final.kvs.len; j++) {
            struct gbs_kv kv = tests[i].final.kvs.fields[j];
            printf("[%s, %zu]", kv.k, kv.v);
        }
        printf("\n");
        printf("\tFinal RAM States:");
        for (size_t j = 0; j < tests[i].initial.rams.len; j++) {
            struct ram_state rs = tests[i].final.rams.states[j];
            printf("[%zu, %zu]", rs.pos, rs.val);
        }
        printf("\n\n");
    }
}
