/* vdispatch.c - generated code, do not edit */

#include "vq_conf.h"

static vq_Type MetaCmd_V (vq_Item A[]) {
  A[0].o.a.v = MetaVop(A[0].o.a.v);
  return VQ_view;
}

#ifdef VQ_OPDEF_H

static vq_Type PassCmd_V (vq_Item A[]) {
  A[0].o.a.v = PassVop(A[0].o.a.v);
  return VQ_view;
}

static vq_Type StepCmd_Iiiis (vq_Item A[]) {
  A[0].o.a.v = StepVop(A[0].o.a.i,A[1].o.a.i,A[2].o.a.i,A[3].o.a.i,A[4].o.a.s);
  return VQ_view;
}

#endif

CmdDispatch f_vdispatch[] = {
  { "meta", "V:V", MetaCmd_V },
#ifdef VQ_OPDEF_H
  { "pass", "V:V", PassCmd_V },
  { "step", "V:Iiiis", StepCmd_Iiiis },
#endif
  {NULL,NULL,NULL}
};

/* end of generated code */
