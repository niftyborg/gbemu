#include "instr.h"

#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include "util.h"

#define mop(instr, match, mask) ((instr & mask) == match)
// Intruction Internals
#define II static inline

static int dispatch_op_word (struct gbstate *ctx);
static int dispatch_op_dword (struct gbstate *ctx);

// 0 false, non-zero false
// low: if true, check lsbits, else check msbits
II int
inrange_ (uint8_t val, uint8_t lb, uint8_t ub, uint8_t linear)
{
        uint8_t mask = 0;
        uint8_t shift = 0;
        if (!linear) {
                mask = 0xF0;
                shift = 4;
        }
        uint8_t test_val = (val & mask) >> shift;
        return test_val >= lb && test_val <= ub;
}

#define inrange(op, LB, UB) (inrange_ ((op), (LB), (UB), 1))
#define inrangev(op, LB, UB) (inrange_ ((op), (LB), (UB), 0))

// cycle position change
struct cpc {
        uint8_t cd; // cycle delta
        uint8_t po; // position offset
};

// update cycle and offset
II void
udo (struct cpc *cpc, uint8_t po, uint8_t cd)
{
        cpc->po = po;
        cpc->cd = cd;
}

// "reconcile" pair
II uint16_t
rpair (struct gbstate *ctx, enum GPR dh, enum GPR dl)
{
        uint16_t h = ctx->reg[dh];
        uint16_t l = ctx->reg[dl];
        uint16_t pair = (h << 8) | l;
        return pair;
}

// "split" pair
II void
spair (struct gbstate *ctx, enum GPR dh, enum GPR dl, uint16_t sp)
{
        uint8_t l = sp & 0xFF;
        uint8_t h = (sp >> 8) & 0xFF;
        ctx->reg[dh] = h;
        ctx->reg[dl] = l;
}

/* int */
/* ld (struct gbstate *ctx) */
/* { */
/*         uint8_t instr = ctx->ram[ctx->pc]; */
/*         uint8_t bits_to_reg_idx[] = {RB, RC, RD, RE, RH, RL, 255, RA}; */
/*         uint8_t source_reg_idx = bits_to_reg_idx[(instr & 0x07)]; */
/*         uint8_t source; */
/*         uint16_t HL = (ctx->reg[RH] << 8) + (ctx->reg[RL]); */
/*         if(source_reg_idx == 255){ */
/*                 source = ctx->ram[HL]; */
/*         } else{ */
/*                 source = ctx->reg[source_reg_idx]; */
/*         } */

/*         uint8_t target_reg_idx = bits_to_reg_idx[((instr >> 3) & 0x7)]; */

/*         if(target_reg_idx == 255){ */
/*                 ctx->ram[HL] = source; */
/*         } else{ */
/*                 ctx->reg[target_reg_idx] = source; */
/*         } */
/* } */

/* int */
/* sub (struct gbstate *ctx) */
/* { */
/*         uint8_t instr = ctx->ram[ctx->pc]; */
/*         uint8_t source_reg = 0; */
/*         switch (instr) { */
/*                 case 0x90: { source_reg = ctx->reg[RB]; } break; */
/*                 case 0x91: { source_reg = ctx->reg[RC]; } break; */
/*                 case 0x92: { source_reg = ctx->reg[RD]; } break; */
/*                 case 0x93: { source_reg = ctx->reg[RE]; } break; */
/*                 case 0x94: { source_reg = ctx->reg[RH]; } break; */
/*                 case 0x95: { source_reg = ctx->reg[RL]; } break; */
/*                 /\* case 0x96: assert(0 && "Unsupported sub istr"); // needs HL *\/ */
/*                 case 0x97: { source_reg = ctx->reg[RA]; } break; */
/*                 default: */
/*                         error ("Unsupported sub instruction %02X", instr); */
/*                         assert(0); */
/*         }; */
/*         ctx->reg[RA] -= source_reg; */
/*         ctx->pc += 1; */
/* } */

int
dispatch_op (struct gbstate *ctx)
{
        int err = 0;
        uint8_t instr = ctx->ram[ctx->pc];
        switch (instr) {
                case 0xD3:
                case 0xE3:
                case 0xE4:
                case 0xF4:
                case 0xDB:
                case 0xEB:
                case 0xEC:
                case 0xFC:
                case 0xDD:
                case 0xDE:
                case 0xDF: {
                        error ("Unsupported GB instruction %02X", instr);
                        return 1;
                } break;
                case 0xCB: {
                        err = dispatch_op_dword (ctx);
                } break;
                default: {
                        err = dispatch_op_word (ctx);
                } break;
        }
        return err;
}

static int
dispatch_op_word (struct gbstate *ctx)
{
        int err = 0;
        uint8_t word = ctx->ram[ctx->pc];
        switch (word) {
                case 0x00: err = nop (ctx); break;
                case 0x10: err = stop (ctx); break;
                case 0x20:
                case 0x30: err = jr (ctx); break;
                case 0x01:
                case 0x11:
                case 0x21:
                case 0x31:
                case 0x02:
                case 0x12:
                case 0x22:
                case 0x32: err = ld (ctx); break;
                case 0x03:
                case 0x13:
                case 0x23:
                case 0x33:
                case 0x04:
                case 0x14:
                case 0x24:
                case 0x34: err = inc (ctx); break;
                case 0x05:
                case 0x15:
                case 0x25:
                case 0x35: err = dec (ctx); break;
                case 0x06:
                case 0x16:
                case 0x26:
                case 0x36: err = ld (ctx); break;
                case 0x07:
                case 0x17: err = rol (ctx); break;
                case 0x27: err = daa (ctx); break;
                case 0x37: err = scf (ctx); break;
                case 0x08: err = ld (ctx); break;
                case 0x18:
                case 0x28:
                case 0x38: err = jr (ctx); break;
                case 0x09:
                case 0x19:
                case 0x29:
                case 0x39: err = add (ctx); break;
                case 0x0A:
                case 0x1A:
                case 0x2A:
                case 0x3A: err = ld (ctx); break;
                case 0x0B:
                case 0x1B:
                case 0x2B:
                case 0x3B: err = dec (ctx); break;
                case 0x0C:
                case 0x1C:
                case 0x2C:
                case 0x3C: err = inc (ctx); break;
                case 0x0D:
                case 0x1D:
                case 0x2D:
                case 0x3D: err = dec (ctx); break;
                case 0x0E:
                case 0x1E:
                case 0x2E:
                case 0x3E: err = ld (ctx); break;
                case 0x0F:
                case 0x1F: err = ror (ctx); break;
                case 0x2F: err = cpl (ctx); break;
                case 0x3F: err = ccf (ctx); break;
                case 0x40: case 0x50: case 0x60:
                case 0x41: case 0x51: case 0x61:
                case 0x42: case 0x52: case 0x62:
                case 0x43: case 0x53: case 0x63:
                case 0x44: case 0x54: case 0x64:
                case 0x45: case 0x55: case 0x65:
                case 0x46: case 0x56: case 0x66:
                case 0x47: case 0x57: case 0x67:
                case 0x48: case 0x58: case 0x68:
                case 0x49: case 0x59: case 0x69:
                case 0x4A: case 0x5A: case 0x6A:
                case 0x4B: case 0x5B: case 0x6B:
                case 0x4C: case 0x5C: case 0x6C:
                case 0x4D: case 0x5D: case 0x6D:
                case 0x4E: case 0x5E: case 0x6E:
                case 0x4F: case 0x5F: case 0x6F: err = ld (ctx); break;
                case 0x70:
                case 0x71:
                case 0x72:
                case 0x73:
                case 0x74:
                case 0x75: err = ld (ctx); break;
                case 0x76: err = halt (ctx); break;
                case 0x77:
                case 0x78:
                case 0x79:
                case 0x7A:
                case 0x7B:
                case 0x7C:
                case 0x7D:
                case 0x7E:
                case 0x7F: err = ld (ctx); break;
                case 0x80:
                case 0x81:
                case 0x82:
                case 0x83:
                case 0x84:
                case 0x85:
                case 0x86:
                case 0x87:
                case 0x88:
                case 0x89:
                case 0x8A:
                case 0x8B:
                case 0x8C:
                case 0x8D:
                case 0x8E:
                case 0x8F: err = add (ctx); break;
                case 0x90:
                case 0x91:
                case 0x92:
                case 0x93:
                case 0x94:
                case 0x95:
                case 0x96:
                case 0x97:
                case 0x98:
                case 0x99:
                case 0x9A:
                case 0x9B:
                case 0x9C:
                case 0x9D:
                case 0x9E:
                case 0x9F: err = sub (ctx); break;
                case 0xA0:
                case 0xA1:
                case 0xA2:
                case 0xA3:
                case 0xA4:
                case 0xA5:
                case 0xA6:
                case 0xA7: err = and (ctx); break;
                case 0xA8:
                case 0xA9:
                case 0xAA:
                case 0xAB:
                case 0xAC:
                case 0xAD:
                case 0xAE:
                case 0xAF: err = xor (ctx); break;
                case 0xB0:
                case 0xB1:
                case 0xB2:
                case 0xB3:
                case 0xB4:
                case 0xB5:
                case 0xB6:
                case 0xB7: err = or (ctx); break;
                case 0xB8:
                case 0xB9:
                case 0xBA:
                case 0xBB:
                case 0xBC:
                case 0xBD:
                case 0xBE:
                case 0xBF: err = cp (ctx); break;
                case 0xC0:
                case 0xD0: err = ret (ctx); break;
                case 0xE0:
                case 0xF0: err = ld (ctx); break;
                case 0xC1:
                case 0xD1:
                case 0xE1:
                case 0xF1: err = pop (ctx); break;
                case 0xC2:
                case 0xD2: err = jp (ctx); break;
                case 0xE2:
                case 0xF2: err = ld (ctx); break;
                case 0xC3: err = jp (ctx); break;
                case 0xF3: err = di (ctx); break;
                case 0xC4:
                case 0xD4: err = call (ctx); break;
                case 0xC5:
                case 0xD5:
                case 0xE5:
                case 0xF5: err = push (ctx); break;
                case 0xC6: err = add (ctx); break;
                case 0xD6: err = sub (ctx); break;
                case 0xE6: err = and (ctx); break;
                case 0xF6: err = or (ctx); break;
                case 0xC7:
                case 0xD7:
                case 0xE7:
                case 0xF7: err = rst (ctx); break;
                case 0xC8:
                case 0xD8: err = ret (ctx); break;
                case 0xE8: err = add (ctx); break;
                case 0xF8: err = ld (ctx); break;
                case 0xC9:
                case 0xD9: err = ret (ctx); break;
                case 0xE9: err = jp (ctx); break;
                case 0xF9: err = ld (ctx); break;
                case 0xCA:
                case 0xDA: err = jp (ctx); break;
                case 0xEA:
                case 0xFA: err = ld (ctx); break;
                case 0xFB: err = ei (ctx); break;
                case 0xCC:
                case 0xDC:
                case 0xCD: err = call (ctx); break;
                case 0xCE: err = add (ctx); break;
                case 0xDE: err = sub (ctx); break;
                case 0xEE: err = xor (ctx); break;
                case 0xFE: err = cp (ctx); break;
                case 0xCF:
                case 0xDF:
                case 0xEF:
                case 0xFF: err = rst (ctx); break;
                default: {
                        error ("Unsupported word instruction: %02X", word);
                        // WHY: dispatch_op should catch this case, if not bad!
                        unreachable ();
                }
        }
        return err;
}

static int
dispatch_op_dword (struct gbstate *ctx)
{
        unreachable ();
}

int
add (struct gbstate *ctx)
{
        unreachable ();
}

int
and (struct gbstate *ctx)
{
        unreachable ();
}

int
call (struct gbstate *ctx)
{
        unreachable ();
}

int
ccf (struct gbstate *ctx)
{
        unreachable ();
}

int
cp (struct gbstate *ctx)
{
        unreachable ();
}

int
cpl (struct gbstate *ctx)
{
        unreachable ();
}

int
daa (struct gbstate *ctx)
{
        unreachable ();
}

int
dec (struct gbstate *ctx)
{
        int err = 0;
        struct cpc co = {0};
        uint8_t op = ctx->ram[ctx->pc];
        enum GPR gpr = 0;
        enum GPR gpr_h = 0;
        enum GPR gpr_l = 0;
        uint8_t a1 = 0;
        uint16_t aa1 = 0;

        switch (op) {
                case 0x35:
                        udo (&co, 1, 3);
                        aa1 = rpair (ctx, RH, RL);
                        ctx->ram[aa1]--;
                        // TODO: handle flags
                        goto processed_instruction;
                case 0x3B:
                        udo (&co, 1, 2);
                        ctx->sp -= 1;
                        goto processed_instruction;
        }

        if (mop (op, 0x04, 0x0F) && inrange (op, 0x0, 0x2)) { // 0x04-0x24
                udo (&co, 1, 2);
                gpr = ((op & 0xF0) >> 4) * 2;
                // TODO: handle flags
                ctx->reg[gpr] -= 1;
        }
        else if (mop (op, 0x0B, 0x0F) && inrangev (op, 0x0, 0x2)) { // 0x0B-0x2B
                // WHY: GPR - B * 2 => [GPR + 0 = GPR_H, GPR + 1 = GPR_L]
                udo (&co, 1, 2);
                gpr_h = ((op & 0xF0) >> 4) * 2;
                gpr_l = gpr_h + 1;
                aa1 = rpair (ctx, gpr_h, gpr_l);
                spair (ctx, gpr_h, gpr_l, aa1 - 1);
        }
        else if (mop (op, 0x0D, 0x0F) && inrange (op, 0x0, 0x3)) { // 0x0D-0x3D
                udo (&co, 1, 1);
                gpr = RC + ((op & 0xF0) >> 4) * 2;
                // TODO: handle flags
                ctx->reg[gpr] -= 1;
        }

processed_instruction:
        if (err == 0) {
                ctx->pc += co.po;
                ctx->cycle += co.cd;
        }
        return err;
}

int
di (struct gbstate *ctx)
{
        unreachable ();
}

int
ei (struct gbstate *ctx)
{
        unreachable ();
}

int
halt (struct gbstate *ctx)
{
        unreachable ();
}

int
inc (struct gbstate *ctx)
{
        int err = 0;
        struct cpc co = {0};
        uint8_t op = ctx->ram[ctx->pc];
        enum GPR gpr = 0;
        enum GPR gpr_h = 0;
        enum GPR gpr_l = 0;
        uint8_t a1 = 0;
        uint16_t aa1 = 0;

        switch (op) {
                case 0x33:
                        udo (&co, 1, 2);
                        ctx->sp += 1;
                        goto processed_instruction;
                case 0x34:
                        udo (&co, 1, 3);
                        aa1 = rpair (ctx, RH, RL);
                        ctx->ram[aa1]++;
                        // TODO: handle flags
                        goto processed_instruction;
        }

        if (mop (op, 0x03, 0x0F) && inrangev (op, 0x0, 0x2)) { // 0x03-0x23
                // WHY: GPR - B * 2 => [GPR + 0 = GPR_H, GPR + 1 = GPR_L]
                udo (&co, 1, 2);
                gpr_h = ((op & 0xF0) >> 4) * 2;
                gpr_l = gpr_h + 1;
                aa1 = rpair (ctx, gpr_h, gpr_l);
                spair (ctx, gpr_h, gpr_l, aa1 + 1);
        }
        else if (mop (op, 0x04, 0x0F) && inrange (op, 0x0, 0x2)) { // 0x04-0x24
                udo (&co, 1, 2);
                gpr = ((op & 0xF0) >> 4) * 2;
                // TODO: handle flags
                ctx->reg[gpr] += 1;
        }
        else if (mop (op, 0x0D, 0x0F) && inrange (op, 0x0, 0x3)) { // 0x0D-0x3D
                udo (&co, 1, 1);
                gpr = RC + ((op & 0xF0) >> 4) * 2;
                // TODO: handle flags
                ctx->reg[gpr] += 1;
        }

processed_instruction:
        if (err == 0) {
                ctx->pc += co.po;
                ctx->cycle += co.cd;
        }
        return err;
}

int
jp (struct gbstate *ctx)
{
        unreachable ();
}

int
jr (struct gbstate *ctx)
{
        unreachable ();
}

II uint8_t
imm8 (struct gbstate *ctx)
{
        return ctx->ram[ctx->pc + 1];
}

II uint16_t
imm16 (struct gbstate *ctx)
{
        uint16_t l = ctx->ram[ctx->pc + 1];
        uint16_t h = ctx->ram[ctx->pc + 2];
        return (h << 8) | l;
}

II uint8_t
mem8 (struct gbstate *ctx, uint16_t addr)
{
        return ctx->ram[addr];
}

II void
ld_r16 (struct gbstate *ctx, enum GPR dl, enum GPR dr, enum GPR sr)
{
        uint16_t pos = rpair (ctx, dl, dr);
        ctx->ram[pos] = ctx->reg[sr];
}

II void
ld_r8r8 (struct gbstate *ctx, enum GPR d, enum GPR s)
{
        ctx->reg[d] = ctx->reg[s];
}

II void
ld_r8m8 (struct gbstate *ctx, enum GPR d)
{
        uint16_t s = rpair (ctx, RH, RL);
        ctx->reg[s] = ctx->ram[s];
}

II void
ld_imm8 (struct gbstate *ctx, enum GPR d)
{
        ctx->reg[d] = ctx->ram[ctx->pc + 1];
}

II void
ld_imm16 (struct gbstate *ctx, enum GPR dh, enum GPR dl)
{
        uint16_t v = imm16 (ctx);
        uint16_t pair = rpair (ctx, dh, dl);
        pair = v;
        spair (ctx, dh, dl, pair);
}

int
ld (struct gbstate *ctx)
{
        int err = 0;
        struct cpc co = {0};
        uint8_t op = ctx->ram[ctx->pc];
        uint16_t pair = 0;
        uint8_t gpr = 0;
        uint8_t a1 = 0; // temporary values
        uint8_t a2 = 0;
        uint16_t aa1 = 0;
        int8_t s1 = 0;

        // Non-ranges
        switch (op) {
                case 0x01:
                        udo (&co, 3, 3);
                        ld_imm16 (ctx, RB, RC);
                        goto processed_instruction;
                case 0x11:
                        udo (&co, 3, 3);
                        ld_imm16 (ctx, RD, RE);
                        goto processed_instruction;
                case 0x21:
                        udo (&co, 3, 3);
                        ld_imm16 (ctx, RH, RL);
                        goto processed_instruction;
                case 0x31:
                        udo (&co, 3, 3);
                        ctx->sp = imm16 (ctx);
                        goto processed_instruction;
                case 0x02:
                        udo (&co, 1, 2);
                        ld_r16 (ctx, RB, RC, RA);
                        goto processed_instruction;
                case 0x12:
                        udo (&co, 1, 2);
                        ld_r16 (ctx, RD, RE, RA);
                        goto processed_instruction;
                case 0x22:
                        udo (&co, 1, 2);
                        ld_r16 (ctx, RH, RL, RA);
                        // NOTE: docs unclear, but increment should happen after changing state (most likely the expected behavior by user)
                        pair = rpair (ctx, RH, RL);
                        spair (ctx, RH, RL, pair + 1);
                        goto processed_instruction;
                case 0x32:
                        udo (&co, 1, 2);
                        ld_r16 (ctx, RH, RL, RA);
                        // TODO: underflow check?
                        pair = rpair (ctx, RH, RL);
                        spair (ctx, RH, RL, pair - 1);
                        goto processed_instruction;
                case 0x06:
                        udo (&co, 2, 2);
                        ld_imm8 (ctx, RB);
                        goto processed_instruction;
                case 0x16:
                        udo (&co, 2, 2);
                        ld_imm8 (ctx, RD);
                        goto processed_instruction;
                case 0x26:
                        udo (&co, 2, 2);
                        ld_imm8 (ctx, RH);
                        goto processed_instruction;
                case 0x36:
                        udo (&co, 2, 3);
                        a1 = ctx->ram[ctx->pc + 1];
                        pair = rpair (ctx, RH, RL);
                        ctx->ram[pair] = a1;
                        goto processed_instruction;
                case 0x08:
                        udo (&co, 3, 5);
                        aa1 = imm16 (ctx);
                        a1 = ctx->sp & 0x00FF;
                        ctx->ram[aa1] = a1;
                        a1 = (ctx->sp & 0xFF00) >> 8;
                        ctx->ram[aa1] = a1;
                        goto processed_instruction;
                case 0x0A:
                        udo (&co, 1, 2);
                        pair = rpair (ctx, RB, RC);
                        ctx->reg[RA] = ctx->ram[pair];
                        goto processed_instruction;
                case 0x1A:
                        udo (&co, 1, 2);
                        pair = rpair (ctx, RD, RE);
                        ctx->reg[RA] = ctx->ram[pair];
                        goto processed_instruction;
                case 0x2A:
                        udo (&co, 1, 2);
                        pair = rpair (ctx, RH, RL);
                        ctx->reg[RA] = ctx->ram[pair];
                        spair (ctx, RH, RL, pair + 1);
                        goto processed_instruction;
                case 0x3A:
                        udo (&co, 1, 2);
                        pair = rpair (ctx, RH, RL);
                        ctx->reg[RA] = ctx->ram[pair];
                        spair (ctx, RH, RL, pair - 1);
                        goto processed_instruction;
                case 0x0E:
                        udo (&co, 2, 2);
                        ld_imm8 (ctx, RC);
                        goto processed_instruction;
                case 0x1E:
                        udo (&co, 2, 2);
                        ld_imm8 (ctx, RE);
                        goto processed_instruction;
                case 0x2E:
                        udo (&co, 2, 2);
                        ld_imm8 (ctx, RL);
                        goto processed_instruction;
                case 0x3E:
                        udo (&co, 2, 2);
                        ld_imm8 (ctx, RA);
                        goto processed_instruction;
                case 0x46:
                        udo (&co, 1, 2);
                        pair = rpair (ctx, RH, RL);
                        a1 = mem8 (ctx, pair);
                        ctx->reg[RB] = a1;
                        goto processed_instruction;
                case 0x56:
                        udo (&co, 1, 2);
                        pair = rpair (ctx, RH, RL);
                        a1 = mem8 (ctx, pair);
                        ctx->reg[RD] = a1;
                        goto processed_instruction;
                case 0x66:
                        udo (&co, 1, 2);
                        pair = rpair (ctx, RH, RL);
                        a1 = mem8 (ctx, pair);
                        ctx->reg[RH] = a1;
                        goto processed_instruction;
                case 0x4E:
                        udo (&co, 1, 2);
                        pair = rpair (ctx, RH, RL);
                        a1 = mem8 (ctx, pair);
                        ctx->reg[RC] = a1;
                        goto processed_instruction;
                case 0x5E:
                        udo (&co, 1, 2);
                        pair = rpair (ctx, RH, RL);
                        a1 = mem8 (ctx, pair);
                        ctx->reg[RE] = a1;
                        goto processed_instruction;
                case 0x6E:
                        udo (&co, 1, 2);
                        pair = rpair (ctx, RH, RL);
                        a1 = mem8 (ctx, pair);
                        ctx->reg[RL] = a1;
                        goto processed_instruction;
                case 0x7E:
                        udo (&co, 1, 2);
                        pair = rpair (ctx, RH, RL);
                        a1 = mem8 (ctx, pair);
                        ctx->reg[RA] = a1;
                        goto processed_instruction;
                case 0xE0:
                        udo (&co, 2, 3);
                        a1 = imm8 (ctx);
                        aa1 = 0xFF00 + (uint16_t) a1;
                        ctx->ram[aa1] = ctx->reg[RA];
                        goto processed_instruction;
                case 0xF0:
                        udo (&co, 2, 3);
                        a1 = imm8 (ctx);
                        aa1 = 0xFF00 + (uint16_t) a1;
                        ctx->reg[RA] = ctx->ram[aa1];
                        goto processed_instruction;
                case 0xE2:
                        udo (&co, 1, 2);
                        a1 = ctx->reg[RC];
                        aa1 = 0xFF00 + (uint16_t) a1;
                        ctx->ram[aa1] = ctx->reg[RA];
                        goto processed_instruction;
                case 0xF2:
                        udo (&co, 1, 2);
                        a1 = ctx->reg[RC];
                        aa1 = 0xFF00 + (uint16_t) a1;
                        ctx->reg[RA] = ctx->ram[aa1];
                        goto processed_instruction;
                case 0xF8:
                        udo (&co, 2, 3);
                        s1 = imm8 (ctx);
                        // TODO: decide if over/under flow check is useful
                        // TODO: flags are updated
                        aa1 = ctx->sp + s1;
                        spair (ctx, RH, RL, aa1);
                        goto processed_instruction;
                case 0xF9:
                        udo (&co, 1, 2);
                        aa1 = rpair (ctx, RH, RL);
                        ctx->sp = aa1;
                        goto processed_instruction;
                case 0xEA:
                        udo (&co, 3, 4);
                        error ("%02X: not sure what it does", op);
                        unreachable ();
                        goto processed_instruction;
                case 0xFA:
                        udo (&co, 3, 4);
                        error ("%02X: not sure what it does", op);
                        unreachable ();
                        goto processed_instruction;
        };

        // WHY: ranges expects that these cases are handled, otherwise
        // interpretation of the operation will be incorrect and lead to UB
        switch (op) {
                case 0x46: case 0x56: case 0x66:
                case 0x4E: case 0x5E: case 0x6E: case 0x7E:
                        // TODO: proper error check, for now this while working
                        // out overall structure and error types
                        error ("Instruction should be processed already %02X", op);
                        unreachable ();
                case 0x76: // Halt instruction, should never be sent here
                        error ("Not a load instruction %02X", op);
                        unreachable ();
        };

        uint8_t op_idx = 0;
        // ranges
        if (inrange (op, 0x40, 0x47)) {
                udo (&co, 1, 1);
                gpr = op & 0x0F;
                ld_r8r8 (ctx, RB, gpr);
        }
        else if (inrange (op, 0x48, 0x4F)) {
                udo (&co, 1, 1);
                gpr = (op & 0x0F) - 0x08;
                ld_r8r8 (ctx, RC, gpr);
        }
        else if (inrange (op, 0x50, 0x57)) {
                udo (&co, 1, 1);
                gpr = op & 0x0F;
                ld_r8r8 (ctx, RD, gpr);
        }
        else if (inrange (op, 0x58, 0x5F)) {
                udo (&co, 1, 1);
                gpr = (op & 0x0F) - 0x08;
                ld_r8r8 (ctx, RE, gpr);
        }
        else if (inrange (op, 0x60, 0x67)) {
                udo (&co, 1, 1);
                gpr = op & 0x0F;
                ld_r8r8 (ctx, RH, gpr);
        }
        else if (inrange (op, 0x68, 0x6F)) {
                udo (&co, 1, 1);
                gpr = (op & 0x0F) - 0x08;
                ld_r8r8 (ctx, RL, gpr);
        }
        else if (inrange (op, 0x70, 0x77)) {
                udo (&co, 1, 2);
                gpr = (op & 0x0F);
                ld_r8m8 (ctx, gpr);
        }
        else if (inrange (op, 0x78, 0x7F)) {
                udo (&co, 1, 1);
                gpr = (op & 0x0F) - 0x08;
                ld_r8r8 (ctx, RA, gpr);
        }


processed_instruction:
        if (err == 0) {
                ctx->cycle += co.cd;
                ctx->pc += co.po;
        }
        return err;
}

int
nop (struct gbstate *ctx)
{
        ctx->pc++;
}

int
or (struct gbstate *ctx)
{
        unreachable ();
}

int
pop (struct gbstate *ctx)
{
        unreachable ();
}

int
push (struct gbstate *ctx)
{
        unreachable ();
}

int
ret (struct gbstate *ctx)
{
        unreachable ();
}

int
rol (struct gbstate *ctx)
{
        unreachable ();
}

int
ror (struct gbstate *ctx)
{
        unreachable ();
}

int
rst (struct gbstate *ctx)
{
        unreachable ();
}

int
scf (struct gbstate *ctx)
{
        unreachable ();
}

int
stop (struct gbstate *ctx)
{
        unreachable ();
}

int
sub (struct gbstate *ctx)
{
        unreachable ();
}

int
xor (struct gbstate *ctx)
{
        unreachable ();
}
