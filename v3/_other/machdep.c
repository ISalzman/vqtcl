#include <stdio.h>

int main() {
  char c = '\xff';
  puts("start");
  printf("%d",c);
#ifdef _BIG_ENDIAN
  puts("_BIG_ENDIAN");
#endif
#ifdef _LITTLE_ENDIAN
  puts("_LITTLE_ENDIAN");
#endif
  puts("end");
  return 0;
}
