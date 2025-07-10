#ifndef JTEST_H_
#define JTEST_H_
#include <stdint.h>
#include <stdlib.h>

#define GBR_LEN 0x10000 /* 4096 */

enum GPR {
RB = 0,
RC,
RD,
RE,
RH,
RL,
RF,
RA,
GPR_LEN,
};

// Hardware Register Addresses
enum HRA {
HRA_JOYP = 0xFF00,
HRA_SB,
HRA_SC,
HRA_DIV,
HRA_TMA,
HRA_TAC,
HRA_IF = 0xFF0F,
HRA_NR10,
HRA_NR11,
HRA_NR12,
HRA_NR13,
HRA_NR14,
HRA_NR21 = 0xFF16,
HRA_NR22,
HRA_NR23,
HRA_NR24,
HRA_NR30 = 0xFF1A,
HRA_NR31,
HRA_NR32,
HRA_NR33,
HRA_NR34,
HRA_NR41 = 0xFF20,
HRA_NR42,
HRA_NR43,
HRA_NR44,
HRA_NR50 = 0xFF24,
HRA_NR51,
HRA_NR52,
HRA_WAVE_RAM = 0xFF30,
HRA_WAVE_RAM_END = 0xFF3F,
HRA_LCDC = 0xFF40,
HRA_STAT,
HRA_SCY,
HRA_SCX,
HRA_LY,
// TODO: finish writing this out
HRA_IE = 0xFFFF,
HRA_LEN,
};

struct gbstate {
    uint16_t pc;
    uint16_t sp;
    unsigned char reg[GPR_LEN];
    unsigned char ram[GBR_LEN];
    size_t cycle;
};

struct gbs_kv {
    // NOTE: Not all the initial state values are registers
    char *k;
    size_t v;
};

struct ram_state {
    size_t pos;
    size_t val;
};

struct kvs {
    struct gbs_kv *fields;
    size_t len;
    size_t cap;
};

struct rams {
    struct ram_state *states;
    size_t len;
    size_t cap;
};

struct test_gbstate {
    struct kvs kvs;
    struct rams rams;
};

struct sm83_test {
    char *name;
    struct test_gbstate initial;
    struct test_gbstate final;
};

void kvs_reset(struct kvs *kvs);
int kvs_append(struct kvs *kvs, struct gbs_kv *val);
int kvs_pop(struct kvs *kvs, struct gbs_kv *val);

void rams_reset(struct rams *rams);
int rams_append(struct rams *rams, struct ram_state *val);
int rams_pop(struct rams *rams, struct ram_state *val);

void sm83_test_dump(struct sm83_test *tests, size_t tests_len);
int run_sm83_test(struct sm83_test t);
#endif // JTEST_H_
