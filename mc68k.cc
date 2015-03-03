#include "mc68k.h"
#include <assert.h>
#include <stdio.h>

#define DUMP  printf

typedef MC68K::WORD WORD;
typedef MC68K::LONG LONG;

MC68K::MC68K() {
  clear();
}

MC68K::~MC68K() {
}

void MC68K::setPc(LONG adr) {
  pc = adr;
}

void MC68K::setSp(LONG adr) {
  a[7] = adr;
}

void MC68K::stat() {
  printf("PC:%08x\n", pc);
}

void MC68K::step() {
  LONG opc = pc;
  WORD op = readMem16(pc);
  pc += 2;
  if ((op & 0xf1ff) == 0x203c) {
    int di = (op >> 9) & 7;
    d[di].l = readMem32(pc);
    pc += 4;
    DUMPA(opc, 6);
    DUMP("move.l #$%08x, D%d\n", d[di].l, di);
  } else if ((op & 0xf1f7) == 0x20c0) {
    int di = (op >> 9) & 7;
    int si = op & 7;
    DUMPA(opc, 2);
    DUMP("move.l D%d, (A%d)+\n", si, di);
    writeMem32(a[di], d[si].l);
    a[di] += 4;
  } else if ((op & 0xfff8) == 0x22d8) {
    int si = op & 7;
    DUMPA(opc, 2);
    DUMP("move.l (A%d)+, (A1)+\n", si);
    writeMem32(a[1], readMem32(a[si]));
    a[si] += 4;
    a[1] += 4;
  } else if (op == 0x23fc) {
    LONG src = readMem32(pc);
    pc += 4;
    LONG dst = readMem32(pc);
    pc += 4;
    DUMPA(opc, 10);
    DUMP("move.l #$%08x, $%08x\n", src, dst);
    writeMem32(dst, src);
  } else if ((op & 0xf1ff) == 0x303c) {
    int di = (op >> 9) & 7;
    d[di].w = readMem16(pc);
    pc += 2;
    DUMPA(opc, 4);
    DUMP("move.w #$%04x, D%d\n", d[di].w, di);
  } else if ((op & 0xf1ff) == 0x41f9) {
    int di = (op >> 9) & 7;
    a[di] = readMem32(pc);
    pc += 4;
    DUMPA(opc, 6);
    DUMP("lea $%08x.l, A%d\n", a[di], di);
  } else if (op == 0x46fc) {
    sr = readMem16(pc);
    pc += 2;
    DUMPA(opc, 4);
    DUMP("move #$%04x, SR\n", sr);
  } else if (op == 0x4e70) {
    DUMPA(opc, 2);
    DUMP("reset\n");
  } else if (op == 0x4e75) {
    pc = pop32();
    DUMPA(opc, 2);
    DUMP("rts\n");
  } else if ((op & 0xfff8) == 0x51c8) {
    int si = op & 7;
    SWORD ofs = readMem16(pc);
    pc += 2;
    d[si].w -= 1;
    if (d[si].w != (WORD)(-1))
      pc = (pc - 2) + ofs;
    DUMPA(opc, 4);
    DUMP("dbra D%d, %06x\n", si, (opc + 2) + ofs);
  } else if (op == 0x6100) {
    LONG adr = pc + (SWORD)readMem16(pc);
    push32(pc + 2);
    pc = adr;
    DUMPA(opc, 4);
    DUMP("bsr %06x\n", adr);
  } else if ((op & 0xf100) == 0x7000) {
    int di = (op >> 9) & 7;
    LONG val = op & 0xff;
    if (val >= 0x80)
      val = -256 + val;
    d[di].l = val;
    DUMPA(opc, 2);
    DUMP("moveq #%d, D%d\n", val, di);
  } else if ((op & 0xfff8) == 0x91c8) {
    int si = op & 7;
    a[0] -= a[si];
    DUMPA(opc, 2);
    DUMP("suba.l A%d, A0\n", si);
  } else if ((op & 0xf1f8) == 0xd080) {
    int di = (op >> 9) & 7;
    int si = op & 7;
    d[di].l += d[si].l;
    DUMPA(opc, 2);
    DUMP("add.l D%d, D%d\n", si, di);
  } else {
    DUMPA(opc, 2);
    assert(!"*ERROR* Unimplemented op\n");
  }
}

void MC68K::clear() {
  for (int i = 0; i < 8; ++i) {
    d[i].l = 0;
    a[i] = 0;
  }
  pc = 0;
}

void MC68K::push32(LONG value) {
  writeMem32(a[7] -= 4, value);
}

LONG MC68K::pop32() {
  LONG adr = a[7];
  a[7] += 4;
  return readMem32(adr);
}

WORD MC68K::readMem16(LONG adr) {
  return (readMem8(adr) << 8) | readMem8(adr + 1);
}
LONG MC68K::readMem32(LONG adr) {
  return (readMem8(adr) << 24) | (readMem8(adr + 1) << 16) | (readMem8(adr + 2) << 8) | readMem8(adr + 3);
}

void MC68K::writeMem16(LONG adr, WORD value) {
  writeMem8(adr    , value >> 8);
  writeMem8(adr + 1, value);
}
void MC68K::writeMem32(LONG adr, LONG value) {
  writeMem8(adr    , value >> 24);
  writeMem8(adr + 1, value >> 16);
  writeMem8(adr + 2, value >> 8);
  writeMem8(adr + 3, value);
}

void MC68K::DUMPA(uint32_t adr, int bytes) {
  char buffer[40], *p = buffer;
  p += sprintf(p, "%06x:", adr);
  for (int i = 0; i < bytes; i += 2) {
    p += sprintf(p, " %04x", readMem16(adr + i));
  }
  printf("%-32s  ", buffer);
}
