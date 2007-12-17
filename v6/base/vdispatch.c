/* vdispatch.c - generated code, do not edit */

#include "vq_conf.h"

static vq_Type MetaCmd_V (vq_Item A[]) {
  A[0].o.a.v = MetaVop(A[0].o.a.v);
  return VQ_view;
}

static vq_Type ReplaceCmd_VIIV (vq_Item A[]) {
  A[0].o.a.v = ReplaceVop(A[0].o.a.v,A[1].o.a.i,A[2].o.a.i,A[3].o.a.v);
  return VQ_view;
}

#ifdef VQ_LOAD_H

static vq_Type AsMetaCmd_S (vq_Item A[]) {
  A[0].o.a.v = AsMetaVop(A[0].o.a.s);
  return VQ_view;
}

static vq_Type OpenCmd_S (vq_Item A[]) {
  A[0].o.a.v = OpenVop(A[0].o.a.s);
  return VQ_view;
}

#endif

#ifdef VQ_MUTABLE_H

static vq_Type MutableCmd_V (vq_Item A[]) {
  A[0].o.a.v = MutableVop(A[0].o.a.v);
  return VQ_view;
}

#endif

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
  { "replace", "V:VIIV", ReplaceCmd_VIIV },
#ifdef VQ_LOAD_H
  { "asmeta", "V:S", AsMetaCmd_S },
  { "open", "V:S", OpenCmd_S },
#endif
#ifdef VQ_MUTABLE_H
  { "mutable", "V:V", MutableCmd_V },
#endif
#ifdef VQ_OPDEF_H
  { "pass", "V:V", PassCmd_V },
  { "step", "V:Iiiis", StepCmd_Iiiis },
#endif
  {NULL,NULL,NULL}
};

/* end of generated code */
