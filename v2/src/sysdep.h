/* sysdep.h -=- System-dependent code */

#ifdef WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>

#define pFindExecutable(a,b,c) (GetModuleFileName(0,b,c) ? b : a)

L I getclicks() {
  LARGE_INTEGER t;
  static double f = 0.0;
  if (f == 0.0) {
    QueryPerformanceFrequency(&t);
    f = (double) t.QuadPart / 1000000.0;
  }
  QueryPerformanceCounter(&t);
  return (I) (f * t.LowPart);
}

#else

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

L I getclicks() {
  struct timeval tv;
  struct timezone tz;
  gettimeofday(&tv, &tz);
  return (I) (tv.tv_sec * 1000000 + tv.tv_usec);
}

/* adapted from TclpFindExecutable in "tcl8.4a2/generic/tclUnixFile.c" */
static const char*
pFindExecutable(const char *argv0, char *buf, int len) {
  const char *p, *q;
  struct stat sb;
  if (argv0 == 0 || strlen(argv0) >= len)
    return "";
  if (strchr(argv0, '/') != 0)
    return argv0;
  p = getenv("PATH");
  if (p == 0)
    p = ":/bin:/usr/bin";
  for (;;) {
    if (*p == 0)
      p = ".";
    /* while (*p == ' ') p++; */
    q = p;
    while (*p != ':' && *p != 0)
      p++;
    if (p - q + strlen(argv0) + 2 >= len)
      return argv0;
    strncpy(buf, q, p - q);
    buf[p-q] = 0;
    if (p != q && p[-1] != '/')
      strcat(buf, "/");
    strcat(buf, argv0);
    if (access(buf, X_OK) == 0 && stat(buf, &sb) == 0 && S_ISREG(sb.st_mode))
      return buf;
    if (*p == '\0')
      return argv0;
    ++p;
  }
}

#endif
