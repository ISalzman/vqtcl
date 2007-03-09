/* dynext.c -=- Sample Thrive extension, dynamically loadable, stub-enabled */

#include "t4ext.h"

t4_DEFINE(dynext)(t4_P box, t4_I cmd) {
  t4_P ws = t4_Work(box);
  switch (cmd) {
    case -1:	      /* called once just after extension has been loaded */
      t4_extIni(ws);  /* initialize the stub callback table */
      return 100;     /* extension version (ignored for now) */

    case 0:	      /* myext #0 ( - s ) */
      t4_strPush(ws, "Hello world!\n", ~0);  /* ~0 indicates 0-terminated str */
      break;

    case 1: {	      /* myext #1 ( i - i ) */
      t4_I v = ws[t4_Length(ws)-1].i; /* int value at top of stack */
      t4_nilPush(ws, -1);             /* drop */
      t4_intPush(ws, v * v);          /* push result */
      break;
    }
  }
  return 0; /* std return, anything else signals a request or an error */
}
