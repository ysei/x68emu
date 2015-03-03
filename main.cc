#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#define DUMP  printf

class MC68K {
public:
  typedef uint32_t DWORD;
  typedef uint16_t WORD;
  typedef uint8_t BYTE;
  typedef int16_t SWORD;

  union Reg {
    DWORD l;  // long
#if 1  // For little endian.
    struct {
      WORD w;  // word
      WORD _dummyW1;
    };
    struct {
      BYTE b;  // byte
      BYTE _dummyB1;
      BYTE _dummyB2;
      BYTE _dummyB3;
    };
#else
#error Register structure for big endian not implemented.
#endif
  };

public:
  MC68K() {
    clear();
  }

  void setPc(DWORD adr) {
    pc = adr;
  }

  void setSp(DWORD adr) {
    a[7] = adr;
  }

  void stat() {
    printf("PC:%08x\n", pc);
  }

  void step() {
    DWORD opc = pc;
    WORD op = readMem16(pc);
    pc += 2;
    if ((op & 0xf1ff) == 0x203c) {
      int di = (op >> 9) & 3;
      d[di].l = readMem32(pc);
      pc += 4;
      DUMPA(opc, 6);
      DUMP("move.l #$%08x, D%d\n", d[di].l, di);
    } else if ((op & 0xf1f7) == 0x20c0) {
      int di = (op >> 9) & 3;
      int si = op & 3;
      writeMem32(a[di], d[si].l);
      a[di] += 4;
      DUMPA(opc, 2);
      DUMP("move.l D%d, (A%d)+\n", si, di);
    } else if ((op & 0xf1ff) == 0x303c) {
      int di = (op >> 9) & 3;
      d[di].w = readMem16(pc);
      pc += 2;
      DUMPA(opc, 4);
      DUMP("move.w #$%04x, D%d\n", d[di].w, di);
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
    } else if (op == 0x4ff9) {
      a[7] = readMem32(pc);
      pc += 4;
      DUMPA(opc, 6);
      DUMP("lea $%08x.l, A7\n", a[7]);
    } else if ((op & 0xfff8) == 0x51c8) {
      int si = op & 7;
      SWORD ofs = readMem16(pc);
      pc += 2;
      d[si].l -= 1;
      if (d[si].l != (DWORD)(-1))
        pc = (pc - 2) + ofs;
      DUMPA(opc, 4);
      DUMP("dbra D%d, %06x\n", si, (opc + 2) + ofs);
    } else if (op == 0x6100) {
      DWORD adr = pc + (SWORD)readMem16(pc);
      push32(pc + 2);
      pc = adr;
      DUMPA(opc, 4);
      DUMP("bsr %06x\n", adr);
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
      DUMP("*ERROR* Unimplemented op\n");
      exit(1);
    }
  }

  //protected:
public:
  Reg d[8];  // Data registers.
  DWORD a[8];  // Address registers.
  DWORD pc;    // Program counter.
  WORD sr;     // Status register.

private:
  void clear() {
    for (int i = 0; i < 8; ++i) {
      d[i].l = 0;
      a[i] = 0;
    }
    pc = 0;
  }

  void push32(DWORD value) {
    writeMem32(a[7] -= 4, value);
  }

  DWORD pop32() {
    DWORD adr = a[7];
    a[7] += 4;
    return readMem32(adr);
  }

  virtual BYTE readMem8(DWORD adr) = 0;
  virtual WORD readMem16(DWORD adr) {
    return (readMem8(adr) << 8) | readMem8(adr + 1);
  }
  virtual DWORD readMem32(DWORD adr) {
    return (readMem8(adr) << 24) | (readMem8(adr + 1) << 16) | (readMem8(adr + 2) << 8) | readMem8(adr + 3);
  }

  virtual void writeMem8(DWORD adr, BYTE value) = 0;
  virtual void writeMem16(DWORD adr, WORD value) {
    writeMem8(adr    , value >> 8);
    writeMem8(adr + 1, value);
  }
  virtual void writeMem32(DWORD adr, DWORD value) {
    writeMem8(adr    , value >> 24);
    writeMem8(adr + 1, value >> 16);
    writeMem8(adr + 2, value >> 8);
    writeMem8(adr + 3, value);
  }

  void DUMPA(uint32_t adr, int bytes) {
    char buffer[32], *p = buffer;
    p += sprintf(p, "%06x:", adr);
    for (int i = 0; i < bytes; i += 2) {
      p += sprintf(p, " %04x", readMem16(adr + i));
    }
    printf("%-30s  ", buffer);
  }
};

class X68K : public MC68K {
public:
  X68K(const uint8_t* ipl) {
    this->ipl = ipl;
    this->mem = new BYTE[0x10000];

    setSp((ipl[0x10000] << 24) | (ipl[0x10001] << 16) | (ipl[0x10002] << 8) | ipl[0x10003]);
    setPc((ipl[0x10004] << 24) | (ipl[0x10005] << 16) | (ipl[0x10006] << 8) | ipl[0x10007]);
  }

  virtual BYTE readMem8(DWORD adr) override {
    if (/*0x0000 <= adr &&*/ adr <= 0xffff) {
      return mem[adr];
    }
    if (0xfe0000 <= adr && adr <= 0xffffff) {
      return ipl[adr - 0xfe0000];
    }
    // TODO: Raise bus error.
    return 0;
  }

  virtual void writeMem8(DWORD adr, BYTE value) override {
    if (/*0x0000 <= adr &&*/ adr <= 0xffff) {
      mem[adr] = value;
      return;
    }
    // TODO: Raise bus error.
  }

private:
  const BYTE* ipl;
  BYTE* mem;
};

uint8_t* readFile(const char* fileName, size_t* pSize) {
  if (pSize != nullptr)
    *pSize = 0;

  uint8_t* data = nullptr;
  int fd = open(fileName, O_RDONLY);
  if (fd != -1) {
    FILE* fp = fdopen(fd, "r");
    if (fp != nullptr) {
      struct stat stbuf;
      if (fstat(fd, &stbuf) != -1) {
        long fileSize = stbuf.st_size;
        data = new uint8_t[fileSize];
        if (data != nullptr) {
          size_t readed = fread(data, fileSize, 1, fp);
          if (readed == 1) {
            if (pSize != nullptr)
              *pSize = fileSize;
          } else {
            delete[] data;
            data = nullptr;
          }
        }
      }
      fclose(fp);
    } else {
      close(fd);
    }
  }
  return data;
}

int main() {
  static const char* kIplRomFileName = "X68BIOSE/IPLROM.DAT";
  size_t iplSize;
  uint8_t* ipl = readFile(kIplRomFileName, &iplSize);
  printf("ipl = %p, size = %ld\n", ipl, iplSize);

  X68K x68k(ipl);

  for (;;) {
    //x68k.stat();
    x68k.step();
  }

  return 0;
}
