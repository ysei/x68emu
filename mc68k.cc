#include "mc68k.h"
#include <assert.h>
#include <stdio.h>

#define DUMP(pc, n, fmt, ...)  { dumpOps(pc, n); printf(fmt "\n", ##__VA_ARGS__); fflush(stdout); }

typedef MC68K::BYTE BYTE;
typedef MC68K::WORD WORD;
typedef MC68K::LONG LONG;

constexpr BYTE FLAG_C = 1 << 0;
//constexpr BYTE FLAG_V = 1 << 1;
constexpr BYTE FLAG_Z = 1 << 2;
constexpr BYTE FLAG_N = 1 << 3;

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
  if ((op & 0xf1f8) == 0x0100) {
    int si = op & 7;
    int di = (op >> 9) & 7;
    if ((d[di].l & (1 << (d[si].b & 31))) == 0)
      sr |= FLAG_Z;
    else
      sr &= ~FLAG_Z;
    DUMP(opc, 2, "btst D%d, D%d", si, di);
  } else if (op == 0x13fc) {
    WORD im = readMem16(pc);
    LONG adr = readMem32(pc + 2);
    pc += 6;
    DUMP(opc, 8, "move.b #$%02x, $%08x.l", im & 0xff, adr);
    writeMem8(adr, im);
  } else if ((op & 0xf1ff) == 0x203c) {
    int di = (op >> 9) & 7;
    d[di].l = readMem32(pc);
    pc += 4;
    DUMP(opc, 6, "move.l #$%08x, D%d", d[di].l, di);
  } else if ((op & 0xf1f7) == 0x20c0) {
    int di = (op >> 9) & 7;
    int si = op & 7;
    DUMP(opc, 2, "move.l D%d, (A%d)+", si, di);
    writeMem32(a[di], d[si].l);
    a[di] += 4;
  } else if ((op & 0xfff8) == 0x22d8) {
    int si = op & 7;
    DUMP(opc, 2, "move.l (A%d)+, (A1)+", si);
    writeMem32(a[1], readMem32(a[si]));
    a[si] += 4;
    a[1] += 4;
  } else if (op == 0x23fc) {
    LONG src = readMem32(pc);
    pc += 4;
    LONG dst = readMem32(pc);
    pc += 4;
    DUMP(opc, 10, "move.l #$%08x, $%08x", src, dst);
    writeMem32(dst, src);
  } else if ((op & 0xf1ff) == 0x303c) {
    int di = (op >> 9) & 7;
    d[di].w = readMem16(pc);
    pc += 2;
    DUMP(opc, 4, "move.w #$%04x, D%d", d[di].w, di);
  } else if ((op & 0xf1ff) == 0x41f9) {
    int di = (op >> 9) & 7;
    a[di] = readMem32(pc);
    pc += 4;
    DUMP(opc, 6, "lea $%08x.l, A%d", a[di], di);
  } else if (op == 0x46fc) {
    sr = readMem16(pc);
    pc += 2;
    DUMP(opc, 4, "move #$%04x, SR", sr);
  } else if (op == 0x4e70) {
    DUMP(opc, 2, "reset");
  } else if (op == 0x4e75) {
    pc = pop32();
    DUMP(opc, 2, "rts");
  } else if ((op & 0xfff8) == 0x51c8) {
    int si = op & 7;
    SWORD ofs = readMem16(pc);
    pc += 2;
    d[si].w -= 1;
    if (d[si].w != (WORD)(-1))
      pc = (pc - 2) + ofs;
    DUMP(opc, 4, "dbra D%d, %06x", si, (opc + 2) + ofs);
  } else if (op == 0x6100) {
    LONG adr = pc + (SWORD)readMem16(pc);
    push32(pc + 2);
    pc = adr;
    DUMP(opc, 4, "bsr %06x", adr);
  } else if (op == 0x6600) {
    SWORD ofs = readMem16(pc);
    LONG dest = pc + 2 + ofs;
    if ((sr & FLAG_Z) == 0)
      pc = dest;
    DUMP(opc, 4, "bne %06x", dest);
  } else if ((op & 0xff00) == 0x6600) {
    SWORD ofs = op & 0xff;
    if (ofs >= 0x80)
      ofs = 256 - ofs;
    LONG dest = pc + ofs;
    if ((sr & FLAG_Z) == 0)
      pc = dest;
    DUMP(opc, 2, "bne %06x", dest);
  } else if ((op & 0xf100) == 0x7000) {
    int di = (op >> 9) & 7;
    LONG val = op & 0xff;
    if (val >= 0x80)
      val = -256 + val;
    d[di].l = val;
    DUMP(opc, 2, "moveq #%d, D%d", val, di);
  } else if ((op & 0xfff8) == 0x91c8) {
    int si = op & 7;
    a[0] -= a[si];
    DUMP(opc, 2, "suba.l A%d, A0", si);
  } else if ((op & 0xf1f8) == 0xb108) {
    int si = op & 7;
    int di = (op >> 9) & 7;
    BYTE v1 = readMem8(a[di]);
    BYTE v2 = readMem8(a[si]);
    a[si] += 1;
    a[di] += 1;
    BYTE c = 0;
    // TODO: Check flag is true.
    if (v1 < v2)
      c |= FLAG_C;
    if (v1 == v2)
      c |= FLAG_Z;
    if (((v1 - v2) & 0x80) != 0)
      c |= FLAG_N;
    sr = (sr & 0xff00) | c;
    DUMP(opc, 2, "cmpm.b (A%d)+, (A%d)+", si, di);
  } else if ((op & 0xf1f8) == 0xd080) {
    int di = (op >> 9) & 7;
    int si = op & 7;
    d[di].l += d[si].l;
    DUMP(opc, 2, "add.l D%d, D%d", si, di);
  } else {
    DUMP(opc, 2, "*** ERROR ***");
    assert(!"Unimplemented op");
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

void MC68K::dumpOps(uint32_t adr, int bytes) {
  char buffer[40], *p = buffer;
  p += sprintf(p, "%06x:", adr);
  for (int i = 0; i < bytes; i += 2) {
    p += sprintf(p, " %04x", readMem16(adr + i));
  }
  printf("%-32s  ", buffer);
}
