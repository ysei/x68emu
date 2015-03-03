#ifndef __X68K_H__
#define __X68K_H__

#include "mc68k.h"

class X68K : public MC68K {
public:
  X68K(const uint8_t* ipl);
  virtual ~X68K();

  virtual BYTE readMem8(LONG adr) override;

  virtual void writeMem8(LONG adr, BYTE value) override;

private:
  const BYTE* ipl;
  BYTE* mem;
};

#endif
