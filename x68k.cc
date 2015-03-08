#include "x68k.h"
#include <assert.h>
#include <stdio.h>

typedef MC68K::BYTE BYTE;

X68K::X68K(const uint8_t* ipl) {
  this->ipl = ipl;
  mem = new BYTE[0x10000];
  sram = new BYTE[0x4000];

  setSp((ipl[0x10000] << 24) | (ipl[0x10001] << 16) | (ipl[0x10002] << 8) | ipl[0x10003]);
  setPc((ipl[0x10004] << 24) | (ipl[0x10005] << 16) | (ipl[0x10006] << 8) | ipl[0x10007]);
}

X68K::~X68K() {
  delete[] mem;
  delete[] sram;
}

BYTE X68K::readMem8(LONG adr) {
  if (/*0x0000 <= adr &&*/ adr <= 0xffff) {
    return mem[adr];
  }
  if (0xe80000 <= adr && adr <= 0xe80030) {  // CRTC
    return 0;
  }
  if (0xed0000 <= adr && adr <= 0xed3fff) {
    return sram[adr - 0xed0000];
  }
  if (0xfe0000 <= adr && adr <= 0xffffff) {
    return ipl[adr - 0xfe0000];
  }

  fflush(stdout);
  fflush(stderr);
  fprintf(stderr, "  ADR=[%06x]: ", adr);
  assert(!"readMem failed\n");
  return 0;
}

void X68K::writeMem8(LONG adr, BYTE value) {
  if (/*0x0000 <= adr &&*/ adr <= 0xffff) {  // MAIN RAM
    mem[adr] = value;
    return;
  }
  if (0xe00000 <= adr && adr <= 0xe7ffff) {  // TEXT VRAM
    // TODO:
    return;
  }
  if (0xe80000 <= adr && adr <= 0xe81fff) {  // CRTC
    // TODO:
    return;
  }
  if (0xe82000 <= adr && adr <= 0xe83fff) {  // video
    // TODO:
    return;
  }
  if (0xe84000 <= adr && adr <= 0xe85fff) {  // DMAC
    // TODO:
    return;
  }
  if (0xe86000 <= adr && adr <= 0xe87fff) {  // AREA set
    // TODO:
    return;
  }
  if (0xe88000 <= adr && adr <= 0xe89fff) {  // MFP
    // TODO:
    return;
  }
  if (0xe8a000 <= adr && adr <= 0xe8bfff) {  // Printer
    // TODO:
    return;
  }
  if (0xe8c000 <= adr && adr <= 0xe8dfff) {  // Sys port
    // TODO:
    return;
  }
  if (0xe9a000 <= adr && adr <= 0xe9bfff) {  // i8255
    // TODO:
    return;
  }
  if (0xed0000 <= adr && adr <= 0xed3fff) {  // SRAM
    sram[adr - 0xed0000] = value;
    return;
  }
  if (adr == 0xe8e00d) {  // I/O port
    return;
  }

  fflush(stdout);
  fflush(stderr);
  fprintf(stderr, "  ADR=[%06x]: ", adr);
  assert(!"writeMem failed\n");
}
