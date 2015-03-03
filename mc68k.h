#ifndef __MC68K_H__
#define __MC68K_H__

#include <stdint.h>

class MC68K {
public:
  typedef uint32_t LONG;
  typedef uint16_t WORD;
  typedef uint8_t BYTE;
  typedef int16_t SWORD;

  union Reg {
    LONG l;  // long
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

  void setPc(LONG adr);
  void setSp(LONG adr);

  void step();

//protected:
public:
  Reg d[8];  // Data registers.
  LONG a[8];  // Address registers.
  LONG pc;    // Program counter.
  WORD sr;     // Status register.

private:
  virtual BYTE readMem8(LONG adr) = 0;
  virtual WORD readMem16(LONG adr);
  virtual LONG readMem32(LONG adr);

  virtual void writeMem8(LONG adr, BYTE value) = 0;
  virtual void writeMem16(LONG adr, WORD value);
  virtual void writeMem32(LONG adr, LONG value);

  void clear();
  void stat();

  void push32(LONG value);
  LONG pop32();

  void DUMPA(uint32_t adr, int bytes);
};

#endif
