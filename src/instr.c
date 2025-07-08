#include <assert.h>
#include <stdio.h>

#include "jtest.h"
#include "util.h"


#define mop(instr, match, mask) ((instr & mask) == match)



// 0 1 [x1 x2 x3] [y1 y2 y3]
void ld(uint8_t instr, struct gbstate *s){
    uint8_t bits_to_reg_idx[] = {
        RB,
        RC,
        RD,
        RE,
        RH,
        RL,
        255,
        RA
    };
    uint8_t source_reg_idx = bits_to_reg_idx[(instr & 0x07)];
    uint8_t source;
    uint16_t HL = (s->reg[RH] << 8) + (s->reg[RL]);
    if(source_reg_idx == 255){
        source = s->ram[HL];
    } else{
        source = s->reg[source_reg_idx];
    }

    uint8_t target_reg_idx = bits_to_reg_idx[((instr >> 3) & 0x7)];

    if(target_reg_idx == 255){
        s->ram[HL] = source;
    } else{
        s->reg[target_reg_idx] = source;
    }
}




void step(struct gbstate *s){
    uint8_t instr = s->ram[s->pc++];
    if mop(instr, 0x76, 0xFF) halt();
    else if mop(instr, 0x40, 0xC0) ld(instr, s);
}
