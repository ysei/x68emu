#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "x68k.h"

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

  X68K x68k(ipl);

  for (;;) {
    //x68k.stat();
    x68k.step();
  }

  return 0;
}
