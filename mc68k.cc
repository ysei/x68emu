#include "mc68k.h"
#include <assert.h>
#include <stdio.h>

#define DUMP(pc, n, fmt, ...)  { dumpOps(pc, n); printf(fmt "\n", ##__VA_ARGS__); fflush(stdout); }

typedef MC68K::BYTE BYTE;
typedef MC68K::WORD WORD;
typedef MC68K::LONG LONG;

constexpr BYTE FLAG_C = 1 << 0;
constexpr BYTE FLAG_V = 1 << 1;
constexpr BYTE FLAG_Z = 1 << 2;
constexpr BYTE FLAG_N = 1 << 3;

constexpr LONG TRAP_VECTOR_START = 0x0080;

static const char kSizeStr[] = {'\0', 'b', 'l', 'w'};
static const char kSizeTable[] = {0, 1, 4, 2};
static const char kDataRegNames[][3] = {"D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7"};
static const char kAdrRegNames[][3] = {"A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7"};
static const char kAdrIndirectNames[][5] = {"(A0)", "(A1)", "(A2)", "(A3)", "(A4)", "(A5)", "(A6)", "(A7)"};
static const char kPostIncAdrIndirectNames[][6] = {"(A0)+", "(A1)+", "(A2)+", "(A3)+", "(A4)+", "(A5)+", "(A6)+", "(A7)+"};
static const char kPreDecAdrIndirectNames[][6] = {"-(A0)", "-(A1)", "-(A2)", "-(A3)", "-(A4)", "-(A5)", "-(A6)", "-(A7)"};
static const char kMoveNames[][6] = {"move", "movea", "move", "move", "move", "move", "move", "move"};

#define NOT_IMPLEMENTED(pc) \
  { DUMP(pc, 2, "*** ERROR ***");               \
    assert(!"Unimplemented op"); }

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
  if ((op & 0xc1ff) == 0x00fc) {  // Except 0x0xxx  (0x1xxx, 0x2xxx, 0x3xxx)
    int size = (op >> 12) & 3;
    int di = (op >> 9) & 7;
    LONG src = fetchImmediate(size);
    DUMP(opc, pc - opc, "move.%c #$%x, (A%d)+", kSizeStr[size], src, di);
    writeValue(a[di], size, src);
    a[di] += kSizeTable[size];
  } else if ((op & 0xcfff) == 0x03fc) {  // Except 0x0xxx  (0x1xxx, 0x2xxx, 0x3xxx)
    int size = (op >> 12) & 3;
    LONG src = fetchImmediate(size);
    LONG dst = readMem32(pc);
    pc += 4;
    DUMP(opc, pc - opc, "move.%c #$%x, $%08x.l", kSizeStr[size], src, dst);
    writeValue(dst, size, src);
  } else if ((op & 0xf1f8) == 0x0100) {
    int si = op & 7;
    int di = (op >> 9) & 7;
    DUMP(opc, pc - opc, "btst D%d, D%d", si, di);
    if ((d[di].l & (1 << (d[si].b & 31))) == 0)
      sr |= FLAG_Z;
    else
      sr &= ~FLAG_Z;
  } else if ((op & 0xf000) == 0x1000) {  // move.b
    char srcBuf[32], dstBuf[32];
    char *srcStr = srcBuf, *dstStr = dstBuf;
    int n = (op >> 9) & 7;
    int m = op & 7;
    int dt = (op >> 6) & 7;
    BYTE src = readSource8((op >> 3) & 7, m, &srcStr);
    writeDestination8(dt, n, src, opc, &dstStr);
    DUMP(opc, pc - opc, "%s.b %s, %s", kMoveNames[dt], srcStr, dstStr);
  } else if ((op & 0xf000) == 0x2000) {  // move.l
    char srcBuf[32], dstBuf[32];
    char *srcStr = srcBuf, *dstStr = dstBuf;
    int n = (op >> 9) & 7;
    int m = op & 7;
    int dt = (op >> 6) & 7;
    LONG src = readSource32((op >> 3) & 7, m, &srcStr);
    writeDestination32(dt, n, src, opc, &dstStr);
    DUMP(opc, pc - opc, "%s.l %s, %s", kMoveNames[dt], srcStr, dstStr);
  } else if ((op & 0xf000) == 0x3000) {  // move.w
    char srcBuf[32], dstBuf[32];
    char *srcStr = srcBuf, *dstStr = dstBuf;
    int n = (op >> 9) & 7;
    int m = op & 7;
    int dt = (op >> 6) & 7;
    WORD src = readSource16((op >> 3) & 7, m, &srcStr);
    writeDestination16(dt, n, src, opc, &dstStr);
    DUMP(opc, pc - opc, "%s.w %s, %s", kMoveNames[dt], srcStr, dstStr);
  } else if ((op & 0xf1ff) == 0x3039) {
    int di = (op >> 9) & 7;
    LONG src = readMem32(pc);
    pc += 4;
    DUMP(opc, pc - opc, "move.w $%08x.l, D%d", src, di);
    d[di].w = readMem16(src);
  } else if ((op & 0xf1ff) == 0x303c) {
    int di = (op >> 9) & 7;
    d[di].w = readMem16(pc);
    pc += 2;
    DUMP(opc, pc - opc, "move.w #$%04x, D%d", d[di].w, di);
  } else if ((op & 0xf1ff) == 0x317c) {
    int di = (op >> 9) & 7;
    WORD src = readMem16(pc);
    SWORD ofs = readMem16(pc + 2);
    pc += 4;
    DUMP(opc, pc - opc, "move.w #$%04x, (%d, A%d)", src, ofs, di);
    writeMem16(a[di] + ofs, src);
  } else if ((op & 0xf1f8) == 0x3040) {
    int si = op & 7;
    int di = (op >> 9) & 7;
    DUMP(opc, pc - opc, "move.w D%d, A%d", si, di);
    a[di] = (SWORD) d[si].w;
  } else if ((op & 0xfff8) == 0x33c0) {
    int si = op & 7;
    LONG dst = readMem32(pc);
    pc += 4;
    DUMP(opc, pc - opc, "move.w D%d, $%08x.l", si, dst);
    writeMem16(dst, d[si].w);
  } else if ((op & 0xf1f8) == 0x3150) {
    int di = (op >> 9) & 7;
    int si = op & 7;
    SWORD ofs = readMem16(pc);
    pc += 2;
    DUMP(opc, pc - opc, "move.w (A%d), (%d, A%d)", si, ofs, di);
    writeMem32(a[di] + ofs, readMem16(a[si]));
  } else if ((op & 0xf1f8) == 0x3168) {
    int di = (op >> 9) & 7;
    int si = op & 7;
    SWORD sofs = readMem16(pc);
    SWORD dofs = readMem16(pc + 2);
    pc += 4;
    DUMP(opc, pc - opc, "move.w (%d, A%d), (%d, A%d)", sofs, si, dofs, di);
    writeMem32(a[di] + dofs, readMem16(a[si] + sofs));
  } else if ((op & 0xf1f8) == 0x41e8) {
    int di = (op >> 9) & 7;
    int si = op & 7;
    SWORD ofs = readMem16(pc);
    pc += 2;
    DUMP(opc, pc - opc, "lea (%d, A%d), A%d", ofs, si, di);
    a[di] = a[si] + ofs;
  } else if ((op & 0xf1ff) == 0x41f9) {
    int di = (op >> 9) & 7;
    a[di] = readMem32(pc);
    pc += 4;
    DUMP(opc, pc - opc, "lea $%08x.l, A%d", a[di], di);
  } else if ((op & 0xfff8) == 0x4268) {
    int si = op & 7;
    SWORD ofs = readMem16(pc);
    pc += 2;
    DUMP(opc, pc - opc, "clr.w (%d, A%d)", ofs, si);
    writeMem16(a[si] + ofs, 0);
  } else if (op == 0x4279) {
    LONG adr = readMem32(pc);
    pc += 4;
    DUMP(opc, pc - opc, "clr.w $%08x.l", adr);
    writeMem16(adr, 0);
  } else if ((op & 0xfff8) == 0x4298) {
    int si = op & 7;
    DUMP(opc, pc - opc, "clr.l (A%d)+", si);
    writeMem32(a[si], 0);
    a[si] += 4;
  } else if (op == 0x46fc) {
    sr = readMem16(pc);
    pc += 2;
    DUMP(opc, pc - opc, "move #$%04x, SR", sr);
  } else if ((op & 0xfff8) == 0x48e0) {
    int di = op & 7;
    WORD bits = readMem16(pc);
    pc += 2;
    DUMP(opc, pc - opc, "movem.l #$%04x, -(A%d)", bits, di);  // TODO: Print registers.
    for (int i = 0; i < 8; ++i) {
      if ((bits & 0x8000) != 0)
        push32(d[i].l);
      bits <<= 1;
    }
    for (int i = 0; i < 8; ++i) {
      if ((bits & 0x8000) != 0)
        push32(a[i]);
      bits <<= 1;
    }
  } else if ((op & 0xfff8) == 0x4a68) {
    int si = op & 7;
    SWORD ofs = readMem16(pc);
    pc += 2;
    DUMP(opc, pc - opc, "tst.w (%d, A%d)", ofs, si);

    SWORD val = readMem16(a[si] + ofs);
    if (val == 0)
      sr |= FLAG_Z;
    else
      sr &= ~FLAG_Z;
    if (val < 0)
      sr |= ~FLAG_N;
    else
      sr &= ~FLAG_N;
    sr &= ~(FLAG_V | FLAG_C);
  } else if ((op & 0xfff8) == 0x4cd8) {
    int si = op & 7;
    WORD bits = readMem16(pc);
    pc += 2;
    DUMP(opc, pc - opc, "movem.l (A%d)+, #$%04x", si, bits);  // TODO: Print registers.
    for (int i = 8; --i >= 0;) {
      if ((bits & 0x8000) != 0)
        a[i] = pop32();
      bits <<= 1;
    }
    for (int i = 8; --i >= 0;) {
      if ((bits & 0x8000) != 0)
        d[i].l = pop32();
      bits <<= 1;
    }
  } else if ((op & 0xfff0) == 0x4e40) {
    int no = op & 0x000f;
    DUMP(opc, pc - opc, "trap #$%x", no);
    // TODO: Move to super visor mode.
    LONG adr = readMem32(TRAP_VECTOR_START + no * 4);
    push32(pc);
    pc = adr;
  } else if (op == 0x4e70) {
    DUMP(opc, pc - opc, "reset");
  } else if (op == 0x4e75) {
    LONG adr = pop32();
    DUMP(opc, pc - opc, "rts");
    pc= adr;
  } else if ((op & 0xfff8) == 0x4e90) {
    int di = op & 7;
    DUMP(opc, pc - opc, "jsr (A%d)", di);
    push32(pc);
    pc = a[di];
  } else if ((op & 0xf1f8) == 0x5088) {
    int ofs = (op >> 9) & 7;
    int si = op & 7;
    ofs = ((ofs - 1) & 7) + 1;
    DUMP(opc, pc - opc, "addq.l #%d, A%d", ofs, si);
    a[si] += ofs;
  } else if ((op & 0xf1f8) == 0x5140) {
    int ofs = (op >> 9) & 7;
    int si = op & 7;
    ofs = ((ofs - 1) & 7) + 1;
    DUMP(opc, pc - opc, "subq.w #%d, D%d", ofs, si);
    d[si].w += ofs;
  } else if ((op & 0xfff8) == 0x51c8) {
    int si = op & 7;
    SWORD ofs = readMem16(pc);
    pc += 2;
    DUMP(opc, pc - opc, "dbra D%d, %06x", si, (pc - 2) + ofs);
    d[si].w -= 1;
    if (d[si].w != (WORD)(-1))
      pc = (pc - 2) + ofs;
  } else if (op == 0x6100) {
    LONG adr = pc + (SWORD)readMem16(pc);
    pc += 2;
    DUMP(opc, pc - opc, "bsr %06x", adr);
    push32(pc);
    pc = adr;
  } else if ((op & 0xff00) == 0x6100) {
    SBYTE ofs = op & 0x00ff;
    DUMP(opc, pc - opc, "bsr %08x", pc + ofs);
    push32(pc);
    pc += ofs;
  } else if (op == 0x6600) {
    SWORD ofs = readMem16(pc);
    pc += 2;
    LONG dest = pc + ofs;
    DUMP(opc, pc - opc, "bne %06x", dest);
    if ((sr & FLAG_Z) == 0)
      pc = dest;
  } else if ((op & 0xff00) == 0x6600) {
    SWORD ofs = op & 0xff;
    if (ofs >= 0x80)
      ofs = 256 - ofs;
    LONG dest = pc + ofs;
    DUMP(opc, pc - opc, "bne %06x", dest);
    if ((sr & FLAG_Z) == 0)
      pc = dest;
  } else if ((op & 0xff00) == 0x6700) {
    SBYTE ofs = op & 0x00ff;
    DUMP(opc, pc - opc, "beq %08x", pc + ofs);
    if ((sr & FLAG_Z) != 0)
      pc += ofs;
  } else if ((op & 0xf100) == 0x7000) {
    int di = (op >> 9) & 7;
    LONG val = op & 0xff;
    if (val >= 0x80)
      val = -256 + val;
    d[di].l = val;
    DUMP(opc, pc - opc, "moveq #%d, D%d", val, di);
  } else if ((op & 0xf1f8) == 0x91c8) {
    int di = (op >> 9) & 7;
    int si = op & 7;
    DUMP(opc, pc - opc, "suba.l A%d, A%d", si, di);
    a[di] -= a[si];
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
    DUMP(opc, pc - opc, "cmpm.b (A%d)+, (A%d)+", si, di);
  } else if ((op & 0xf1ff) == 0xc0bc) {
    int di = (op >> 9) & 7;
    LONG src = readMem32(pc);
    pc += 4;
    DUMP(opc, pc - opc, "and.l #$%08x, D%d", src, di);
    d[di].l &= src;
  } else if ((op & 0xf1f8) == 0xd080) {
    int di = (op >> 9) & 7;
    int si = op & 7;
    DUMP(opc, pc - opc, "add.l D%d, D%d", si, di);
    d[di].l += d[si].l;
  } else if ((op & 0xf1ff) == 0xd0bc) {
    int di = (op >> 9) & 7;
    LONG src = readMem32(pc);
    pc += 4;
    DUMP(opc, pc - opc, "add.l #$%08x, D%d", src, di);
    d[di].l += src;
  } else if ((op & 0xf1f8) == 0xd1c8) {
    int di = (op >> 9) & 7;
    int si = op & 7;
    DUMP(opc, pc - opc, "adda.l A%d, A%d", si, di);
    a[di] += a[si];
  } else if ((op & 0xf1ff) == 0xd1fc) {
    int di = (op >> 9) & 7;
    LONG src = readMem32(pc);
    pc += 4;
    DUMP(opc, pc - opc, "adda.l #$%08x, A%d", src, di);
    a[di] += src;
  } else {
    NOT_IMPLEMENTED(opc);
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

WORD MC68K::readSource16(int type, int m, char** str) {
  switch (type) {
  case 0:  // move.w Dm, xx
    *str = const_cast<char*>(kDataRegNames[m]);
    return d[m].w;
  case 1:  // move.w Am, xx
    *str = const_cast<char*>(kAdrRegNames[m]);
    return a[m];
  case 2:  // move.w (Am), xx
    *str = const_cast<char*>(kAdrIndirectNames[m]);
    return readMem16(a[m]);
  case 3:  // move.w (Am)+, xx
    {
      LONG adr = a[m];
      a[m] += 2;
      *str = const_cast<char*>(kPostIncAdrIndirectNames[m]);
      return readMem16(adr);
    }
  case 4:  // move.w -(Am), xx
    a[m] -= 2;
    *str = const_cast<char*>(kPreDecAdrIndirectNames[m]);
    return readMem16(a[m]);
  case 5:  // move.w ($123,Am), xx
    {
      SWORD ofs = readMem16(pc);
      pc += 2;
      sprintf(*str, "(%d, A%d)", ofs, m);
      return readMem16(a[m] + ofs);
    }
  case 6:  // move.w ([$4567,A0,D0.w],-$3211), xx
    break;
  case 7:  // Misc.
    switch (m) {
    case 0:  // move.w $XXXX.w, xx
      {
        WORD adr = readMem16(pc);
        pc += 2;
        sprintf(*str, "$%04x", adr);
        return readMem16(adr);
      }
    case 1:  // move.w $XXXXXXXX.l, xx
      {
        LONG adr = readMem32(pc);
        pc += 4;
        sprintf(*str, "$%08x", adr);
        return readMem16(adr);
      }
    case 2:  // move.w ($XXXX, PC), xx
      {
        SWORD ofs = readMem16(pc);
        pc += 2;
        sprintf(*str, "(%d, PC)", ofs);
        return readMem16(pc + ofs);
      }
    case 3:  // move.w ([$XXXX,A0,D0.w],$YYYY), xx
      break;
    case 4:  // move.w #$XXXX, xx
      {
        WORD src = readMem16(pc);
        pc += 2;
        sprintf(*str, "#$%04x", src);
        return src;
      }
    default:
      break;
    }
  default:
    break;
  }
  NOT_IMPLEMENTED(pc - 2);
  return 0;
}

void MC68K::writeDestination16(int type, int n, WORD src, LONG opc, char** str) {
  switch (type) {
  case 0:  // move.w xx, Dn
    d[n].w = src;
    *str = const_cast<char*>(kDataRegNames[n]);
    return;
  case 1:  // move.w xx, An
    a[n] = (SWORD) src;
    *str = const_cast<char*>(kAdrRegNames[n]);
    return;
  case 2:  // move.w xx, (An)
    writeMem16(a[n], src);
    *str = const_cast<char*>(kAdrIndirectNames[n]);
    return;
  case 3:  // move.w xx, (An)+
    writeMem16(a[n], src);
    a[n] += 2;
    *str = const_cast<char*>(kPostIncAdrIndirectNames[n]);
    return;
  case 4:  // move.w xx, -(An)
    a[n] -= 2;
    writeMem16(a[n], src);
    *str = const_cast<char*>(kPreDecAdrIndirectNames[n]);
    return;
  case 5:  // move.w xx, ($123,An)
    {
      SWORD ofs = readMem16(pc);
      pc += 2;
      writeMem16(a[n] + ofs, src);
      sprintf(*str, "(%d, A%d)", ofs, n);
    }
    return;
  case 6:  // move.w xx, ([$4567,A0,D0.w],-$3211)
    break;
  case 7:  // move.w xx, $XXXX.w
    switch (n) {
    case 0:
      {
        WORD adr = readMem16(pc);
        pc += 2;
        writeMem16(adr, src);
        sprintf(*str, "$%04x", adr);
      }
      return;
    case 1:
      {
        LONG adr = readMem32(pc);
        pc += 4;
        writeMem16(adr, src);
        sprintf(*str, "$%08x", adr);
      }
      return;
    default:
      break;
    }
  default:
    break;
  }
  NOT_IMPLEMENTED(opc);
}

LONG MC68K::readSource32(int type, int m, char** str) {
  switch (type) {
  case 0:  // move.l Dm, xx
    *str = const_cast<char*>(kDataRegNames[m]);
    return d[m].l;
  case 1:  // move.l Am, xx
    *str = const_cast<char*>(kAdrRegNames[m]);
    return a[m];
  case 2:  // move.l (Am), xx
    *str = const_cast<char*>(kAdrIndirectNames[m]);
    return readMem32(a[m]);
  case 3:  // move.l (Am)+, xx
    {
      LONG adr = a[m];
      a[m] += 4;
      *str = const_cast<char*>(kPostIncAdrIndirectNames[m]);
      return readMem32(adr);
    }
  case 4:  // move.l -(Am), xx
    a[m] -= 4;
    *str = const_cast<char*>(kPreDecAdrIndirectNames[m]);
    return readMem32(a[m]);
  case 5:  // move.l ($123,Am), xx
    {
      SWORD ofs = readMem16(pc);
      pc += 2;
      sprintf(*str, "(%d, A%d)", ofs, m);
      return readMem32(a[m] + ofs);
    }
  case 6:  // move.l ([$4567,A0,D0.w],-$3211), xx
    break;
  case 7:  // Misc.
    switch (m) {
    case 0:  // move.l $XXXX.w, xx
      {
        WORD adr = readMem16(pc);
        pc += 2;
        sprintf(*str, "$%04x", adr);
        return readMem32(adr);
      }
    case 1:  // move.l $XXXXXXXX.l, xx
      {
        LONG adr = readMem32(pc);
        pc += 4;
        sprintf(*str, "$%08x", adr);
        return readMem32(adr);
      }
    case 2:  // move.l ($XXXX, PC), xx
      {
        SWORD ofs = readMem16(pc);
        pc += 2;
        sprintf(*str, "(%d, PC)", ofs);
        return readMem32(pc + ofs);
      }
    case 3:  // move.l ([$XXXX,A0,D0.w],$YYYY), xx
      break;
    case 4:  // move.l #$XXXX, xx
      {
        LONG src = readMem32(pc);
        pc += 4;
        sprintf(*str, "#$%08x", src);
        return src;
      }
    default:
      break;
    }
  default:
    break;
  }
  NOT_IMPLEMENTED(pc - 2);
  return 0;
}

void MC68K::writeDestination32(int type, int n, LONG src, LONG opc, char** str) {
  switch (type) {
  case 0:  // move.l xx, Dn
    d[n].l = src;
    *str = const_cast<char*>(kDataRegNames[n]);
    return;
  case 1:  // move.l xx, An
    a[n] = src;
    *str = const_cast<char*>(kAdrRegNames[n]);
    return;
  case 2:  // move.l xx, (An)
    writeMem32(a[n], src);
    *str = const_cast<char*>(kAdrIndirectNames[n]);
    return;
  case 3:  // move.l xx, (An)+
    writeMem32(a[n], src);
    a[n] += 4;
    *str = const_cast<char*>(kPostIncAdrIndirectNames[n]);
    return;
  case 4:  // move.l xx, -(An)
    a[n] -= 4;
    writeMem32(a[n], src);
    *str = const_cast<char*>(kPreDecAdrIndirectNames[n]);
    return;
  case 5:  // move.l xx, ($123,An)
    {
      SWORD ofs = readMem16(pc);
      pc += 2;
      writeMem32(a[n] + ofs, src);
      sprintf(*str, "(%d, A%d)", ofs, n);
    }
    return;
  case 6:  // move.l xx, ([$4567,A0,D0.w],-$3211)
    break;
  case 7:  // move.l xx, $XXXX.w
    switch (n) {
    case 0:
      {
        WORD adr = readMem16(pc);
        pc += 2;
        writeMem32(adr, src);
        sprintf(*str, "$%04x", adr);
      }
      return;
    case 1:
      {
        LONG adr = readMem32(pc);
        pc += 4;
        writeMem32(adr, src);
        sprintf(*str, "$%08x", adr);
      }
      return;
    default:
      break;
    }
  default:
    break;
  }
  NOT_IMPLEMENTED(opc);
}

BYTE MC68K::readSource8(int type, int m, char** str) {
  switch (type) {
  case 0:  // move.b Dm, xx
    *str = const_cast<char*>(kDataRegNames[m]);
    return d[m].b;
  case 1:  // move.b Am, xx
    break;
  case 2:  // move.b (Am), xx
    *str = const_cast<char*>(kAdrIndirectNames[m]);
    return readMem8(a[m]);
  case 3:  // move.b (Am)+, xx
    {
      LONG adr = a[m];
      a[m] += 1;
      *str = const_cast<char*>(kPostIncAdrIndirectNames[m]);
      return readMem8(adr);
    }
  case 4:  // move.b -(Am), xx
    a[m] -= 1;
    *str = const_cast<char*>(kPreDecAdrIndirectNames[m]);
    return readMem8(a[m]);
  case 5:  // move.b ($123,Am), xx
    {
      SWORD ofs = readMem16(pc);
      pc += 2;
      sprintf(*str, "(%d, A%d)", ofs, m);
      return readMem8(a[m] + ofs);
    }
  case 6:  // move.b ([$4567,A0,D0.w],-$3211), xx
    break;
  case 7:  // Misc.
    switch (m) {
    case 0:  // move.b $XXXX.w, xx
      {
        WORD adr = readMem16(pc);
        pc += 2;
        sprintf(*str, "$%04x", adr);
        return readMem8(adr);
      }
    case 1:  // move.b $XXXXXXXX.l, xx
      {
        LONG adr = readMem32(pc);
        pc += 4;
        sprintf(*str, "$%08x", adr);
        return readMem8(adr);
      }
    case 2:  // move.b ($XXXX, PC), xx
      {
        SWORD ofs = readMem16(pc);
        pc += 2;
        sprintf(*str, "(%d, PC)", ofs);
        return readMem8(pc + ofs);
      }
    case 3:  // move.b ([$XXXX,A0,D0.w],$YYYY), xx
      break;
    case 4:  // move.b #$XXXX, xx
      {
        WORD src = readMem16(pc);
        pc += 2;
        sprintf(*str, "#$%04x", src);
        return src;
      }
    default:
      break;
    }
  default:
    break;
  }
  NOT_IMPLEMENTED(pc - 2);
  return 0;
}

void MC68K::writeDestination8(int type, int n, BYTE src, LONG opc, char** str) {
  switch (type) {
  case 0:  // move.b xx, Dn
    d[n].b = src;
    *str = const_cast<char*>(kDataRegNames[n]);
    return;
  case 1:  // move.b xx, An
    break;
  case 2:  // move.b xx, (An)
    writeMem8(a[n], src);
    *str = const_cast<char*>(kAdrIndirectNames[n]);
    return;
  case 3:  // move.b xx, (An)+
    writeMem8(a[n], src);
    a[n] += 1;
    *str = const_cast<char*>(kPostIncAdrIndirectNames[n]);
    return;
  case 4:  // move.b xx, -(An)
    a[n] -= 1;
    writeMem8(a[n], src);
    *str = const_cast<char*>(kPreDecAdrIndirectNames[n]);
    return;
  case 5:  // move.b xx, ($123,An)
    {
      SWORD ofs = readMem16(pc);
      pc += 2;
      writeMem8(a[n] + ofs, src);
      sprintf(*str, "(%d, A%d)", ofs, n);
    }
    return;
  case 6:  // move.b xx, ([$4567,A0,D0.w],-$3211)
    break;
  case 7:  // move.b xx, $XXXX.w
    switch (n) {
    case 0:
      {
        WORD adr = readMem16(pc);
        pc += 2;
        writeMem8(adr, src);
        sprintf(*str, "$%04x", adr);
      }
      return;
    case 1:
      {
        LONG adr = readMem32(pc);
        pc += 4;
        writeMem8(adr, src);
        sprintf(*str, "$%08x", adr);
      }
      return;
    default:
      break;
    }
  default:
    break;
  }
  NOT_IMPLEMENTED(opc);
}
