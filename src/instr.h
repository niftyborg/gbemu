#ifndef INSTR_H_
#define INSTR_H_
#include <assert.h>
#include <stdio.h>
#include "jtest.h"

// Op dispatch
void step (struct gbstate *s);
int dispatch_op (struct gbstate *s);
// WORD size instructions
int add (struct gbstate *s);
int and (struct gbstate *s);
int call (struct gbstate *s);
int ccf (struct gbstate *s);
int cp (struct gbstate *s);
int cpl (struct gbstate *s);
int daa (struct gbstate *s);
int dec (struct gbstate *s);
int di (struct gbstate *s);
int ei (struct gbstate *s);
int halt (struct gbstate *s);
int inc (struct gbstate *s);
int jp (struct gbstate *s);
int jr (struct gbstate *s);
int ld (struct gbstate *s);
int nop (struct gbstate *s);
int or (struct gbstate *s);
int pop (struct gbstate *s);
int push (struct gbstate *s);
int ret (struct gbstate *s);
int rol (struct gbstate *s);
int ror (struct gbstate *s);
int rst (struct gbstate *s);
int scf (struct gbstate *s);
int stop (struct gbstate *s);
int sub (struct gbstate *s);
int xor (struct gbstate *s);
// DOUBLE WORD size instructions

#endif // INSTR_H_
