#ifndef __MC68K_H__
#define __MC68K_H__

#include <stdint.h>

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
  MC68K();
  virtual ~MC68K();

  void setPc(DWORD adr);
  void setSp(DWORD adr);

  void step();

//protected:
public:
  Reg d[8];  // Data registers.
  DWORD a[8];  // Address registers.
  DWORD pc;    // Program counter.
  WORD sr;     // Status register.

private:
  virtual BYTE readMem8(DWORD adr) = 0;
  virtual WORD readMem16(DWORD adr);
  virtual DWORD readMem32(DWORD adr);

  virtual void writeMem8(DWORD adr, BYTE value) = 0;
  virtual void writeMem16(DWORD adr, WORD value);
  virtual void writeMem32(DWORD adr, DWORD value);

  void clear();
  void stat();

  void push32(DWORD value);
  DWORD pop32();

  void DUMPA(uint32_t adr, int bytes);
};

#endif
