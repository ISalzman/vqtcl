/* xdyn.h -=- Load a dynamic library */

#if defined(WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

L I dynExt(P box, I cmd) {
  P w = Work(box), t = w+Wd.i-1;
  I i;
  switch (cmd) {
    case 0: { /* dynload ( s s - i ) */
      S fn = cString(w,-2);
      S ep = cString(w,-1);
      HMODULE lh = LoadLibrary(fn);
      nilPush(w,-2);
      if (lh == 0)
	intPush(w,-1);
      else {
	F pf = (F)(I)GetProcAddress(lh,ep);
	if (pf == 0) {
	  FreeLibrary(lh);
	  intPush(w,-2);
	} else {
	  P bx = extCode(w,ep,pf);
	  pf(bx,-1);
	}
      }
      break;
    }
    case 1: /* dynopts ( - ) */
      break;
    case 2: /* dynopen ( s - h ) */
      i = (I)LoadLibrary(cString(w,-1));
      nilPush(w,-1);
      intPush(w,i);
      break;
    case 3: /* dynerror ( - i ) */
      intPush(w,GetLastError());
      break;
    case 4: /* dynsym ( h s - p ) */
      i = (I)GetProcAddress((HINSTANCE)t[-1].i,cString(w,-1));
      nilPush(w,-2);
      intPush(w,i);
      break;
    case 5: /* dynclose ( h - ) */
      FreeLibrary((HINSTANCE)t->i);
      nilPush(w,-1);
      break;
  }
  return 0;
}

#else

#include <dlfcn.h>

L I dynExt(P box, I cmd) {
  P w = Work(box), t = w+Wd.i-1;
  I i;
  switch (cmd) {
    case 0: { /* dynload ( s s - i ) */
      S fn = cString(w,-2);
      S ep = cString(w,-1);
      void *lh = dlopen(fn,RTLD_NOW|RTLD_GLOBAL);
      nilPush(w,-2);
      if (lh == 0)
	intPush(w,-1);
      else {
	F pf = (F)(I)dlsym(lh,ep);
	if (pf == 0) {
	  dlclose(lh);
	  intPush(w,-2);
	} else {
	  P bx = extCode(w,ep,pf);
	  pf(bx,-1);
	}
      }
      break;
    }
    case 1: /* dynopts ( - i ) */
      intPush(w,RTLD_NOW|RTLD_GLOBAL);
      break;
    case 2: /* dynopen ( s i - h ) */
      i = (I)dlopen(cString(w,-2),t->i);
      nilPush(w,-2);
      intPush(w,i);
      break;
    case 3: /* dynerror ( - s ) */
      strPush(w,dlerror(),~0);
      break;
    case 4: /* dynsym ( h s - p ) */
      i = (I)dlsym((void*)t[-1].i,cString(w,-1));
      nilPush(w,-2);
      intPush(w,i);
      break;
    case 5: /* dynclose ( h - ) */
      dlclose((void*)t->i);
      nilPush(w,-1);
      break;
  }
  return 0;
}

#endif
