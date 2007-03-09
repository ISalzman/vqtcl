/* xmmf.h -=- Memory-mapped file extension */

#ifdef WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

L I mmfExt(P p, I c) { P w = Work(p);
  switch (c) {
    case 0: { /* mmap ( s - s ) */
      HANDLE f = CreateFile(cString(w,-1),GENERIC_READ,FILE_SHARE_READ,
	  		    0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
      Wpopd;
      if (f == INVALID_HANDLE_VALUE) intPush(w,-1); else {
	DWORD n = GetFileSize(f,0);
	if (n == 0) 
	  strPush(w,0,0);
	else {
	  HANDLE h = CreateFileMapping(f,0,PAGE_READONLY,0,0,0);
	  if (h == INVALID_HANDLE_VALUE) intPush(w,-2); else {
	    void* p = MapViewOfFile(h,FILE_MAP_READ,0,0,n);
	    if (p == 0) intPush(w,-3); else strPush(w,p,~n);
	    CloseHandle(h);
	  }
	  CloseHandle(f);
	}
      }
      break;
    }
    case 1: /* munmap ( s - i ) */
      intPush(w,UnmapViewOfFile((char*)strAt(Wpopd)));
      break;
  }
  return 0;
}

#else

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

L I mmfExt(P p, I c) { P w = Work(p);
  switch (c) {
    case 0: { /* mmap ( s - s ) */
      int fd = open(cString(w,-1),O_RDONLY);
      Wpopd;
      if (fd == -1) intPush(w,-1); else {
	struct stat sb;
	if (fstat(fd,&sb) == -1) intPush(w,-2); else {
	  long n = (long) sb.st_size;
	  if (n <= 0) strPush(w,0,0); else {
	    void* p = mmap(0,(size_t)n,PROT_READ,MAP_SHARED,fd,0);
	    if (p == MAP_FAILED) intPush(w,-3); else strPush(w,p,~n);
	  }
	}
	close(fd);
      }
      break;
    }
    case 1: { /* munmap ( s - i ) */
      int i = munmap((void*)strAt(Wtopd),(size_t)lenAt(Wtopd));
      intPush(w,i);
      break;
    }
  }
  return 0;
}

#endif
