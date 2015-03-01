#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

class MC68K {
public:
  typedef uint32_t DWORD;
  typedef uint16_t WORD;
  typedef uint8_t BYTE;
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

  MC68K::DWORD a7 = (ipl[0x10000] << 24) | (ipl[0x10001] << 16) | (ipl[0x10002] << 8) | (ipl[0x10003]);
  MC68K::DWORD pc = (ipl[0x10004] << 24) | (ipl[0x10005] << 16) | (ipl[0x10006] << 8) | (ipl[0x10007]);
  printf("a7 = %08x, pc = %08x\n", a7, pc);

  return 0;
}
