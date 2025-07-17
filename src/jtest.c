#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include <cJSON.h>

#include "jtest.h"
#include "instr.h"
#include "util.h"

static const int key_to_idx[256] = {
    ['a'] = RA, ['f'] = RF, ['b'] = RB, ['c'] = RC, ['d'] = RD, ['e'] = RE,
    ['h'] = RH, ['l'] = RL
};

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

struct gbstate empty_gbstate(){
    struct gbstate state = {0};
    return state;
}

struct gbstate test_gbstate_to_gbstate(struct test_gbstate s){
    struct gbstate to_ret = empty_gbstate();
    struct rams r = s.rams;
    for(size_t i = 0; i < r.len; i++){
        struct ram_state rs = r.states[i];
        to_ret.ram[rs.pos] = rs.val;
    }
    struct kvs k = s.kvs;
    for(size_t i = 0; i < k.len; i++){
        struct gbs_kv kv = k.fields[i];
        if(strcmp(kv.k, "sp") == 0){
            to_ret.sp = kv.v;
        } else if (strcmp(kv.k, "pc") == 0){
            to_ret.pc = kv.v;
        } else if (strcmp(kv.k, "ime") == 0){
        } else {
            uint8_t reg_idx = key_to_idx[(size_t) kv.k[0]];
            to_ret.reg[reg_idx] = kv.v;
        }
    }
    return to_ret;
}

int verify_gbstate_with_test(struct test_gbstate s, struct gbstate s_hat){
    struct rams r = s.rams;
    int mismatches = 0;
    for(size_t i = 0; i < r.len; i++){
        struct ram_state rs = r.states[i];
        mismatches += (s_hat.ram[rs.pos] != rs.val);
    }
    struct kvs k = s.kvs;
    for(size_t i = 0; i < k.len; i++){
        struct gbs_kv kv = k.fields[i];
        if(strcmp(kv.k, "sp") == 0){
            mismatches += (s_hat.sp != kv.v);
        } else if (strcmp(kv.k, "pc") == 0){
            mismatches += (s_hat.pc != kv.v);
        } else if (strcmp(kv.k, "ime") == 0){
        } else {
            uint8_t reg_idx = key_to_idx[(size_t) kv.k[0]];
            mismatches += (s_hat.reg[reg_idx] != kv.v);
        }
    }
    return mismatches;
}

int run_sm83_test(struct sm83_test t){
    struct gbstate s = test_gbstate_to_gbstate(t.initial);
    dispatch_op (&s);
    return verify_gbstate_with_test(t.final, s); // 0 = success
}

void sm83_test_dump(struct sm83_test *tests, size_t tests_len) {
    for (size_t i = 0; i < tests_len; i++) {
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

int parse_file(char *file_path, struct sm83_test **sm83_tests, size_t *length) {
    // TODO: definitely missing some registers, will check later
    #define KNAMES_LEN 12
    static char *knames[KNAMES_LEN] =
        { "pc", "sp", "a", "b", "c", "d", "e", "f", "h", "l", "ime", "ei" };

    // TODO: clean up allocations on errors
    struct gbs_kv kv;
    struct ram_state rstate;
    struct sm83_test st = {0};
    cJSON *test = NULL;
    cJSON *rpair = NULL;
    cJSON *rp_t = NULL;
    struct sm83_test *sm83ts = {0};

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
    sm83ts = calloc (test_len + 1, sizeof(struct sm83_test));
    if (sm83ts == NULL) {
        error("Failed to allocate sm83 tests");
        err = 4;
        goto error;
    }
    size_t test_idx = 0;

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
        cJSON *final_ram = cJSON_GetObjectItemCaseSensitive(final, "ram");
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
    if (tests) cJSON_free (tests);
    return err; // err == 0
error:
    // TODO: cleanup all allocations
    if (st.name != NULL) free (st.name);
    /* if (st.initial */
    if (file_data.str != NULL) free (file_data.str);
    if (sm83ts != NULL) free (sm83ts);
    return err; // err == non-zero
}
