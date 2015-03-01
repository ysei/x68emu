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

public:
  MC68K() {
    clear();
  }

  void boot() {
    a[7] = readMem32(0);
    pc = readMem32(4);
  }

  void stat() {
    printf("PC:%08x\n", pc);
  }

  void step() {
    WORD op = readMem16(pc);
    pc += 2;
    if (op == 0x46fc) {
      sr = readMem16(pc);
      DUMP("move #$%04x, SR\n", sr);
    } else {
      fprintf(stderr, "Unimplemented op: [%04x]\n", op);
      exit(1);
    }
  }

  //protected:
public:
  DWORD d[8];  // Data registers.
  DWORD a[8];  // Address registers.
  DWORD pc;    // Program counter.
  WORD sr;     // Status register.

private:
  void clear() {
    for (int i = 0; i < 8; ++i) {
      d[i] = 0;
      a[i] = 0;
    }
    pc = 0;
  }

  virtual BYTE readMem8(DWORD adr) = 0;
  virtual WORD readMem16(DWORD adr) {
    return (readMem8(adr) << 8) | readMem8(adr + 1);
  }
  virtual DWORD readMem32(DWORD adr) {
    return (readMem8(adr) << 24) | (readMem8(adr + 1) << 16) | (readMem8(adr + 2) << 8) | readMem8(adr + 3);
  }
};

class X68K : public MC68K {
public:
  X68K(const uint8_t* ipl) {
    this->ipl = ipl;
  }

  virtual BYTE readMem8(DWORD adr) {
    if (/*0x0000 <= adr &&*/ adr <= 0xffff) {
      return ipl[adr + 0x10000];
    }
    if (0xfe0000 <= adr && adr <= 0xffffff) {
      return ipl[adr - 0xfe0000];
    }
    // TODO: Raise bus error.
    return 0;
  }

private:
  const BYTE* ipl;
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
  x68k.boot();

  x68k.stat();

  x68k.step();

  x68k.stat();

  return 0;
}
