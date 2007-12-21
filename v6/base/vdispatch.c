/* vdispatch.c - generated code, do not edit */

#include "vq_conf.h"

#if VQ_DISPATCH_H

static vq_Type EmptyCmd_VII (vq_Cell A[]) {
  A[0].o.a.i = EmptyVop(A[0].o.a.v,A[1].o.a.i,A[2].o.a.i);
  return VQ_int;
}

static vq_Type MetaCmd_V (vq_Cell A[]) {
  A[0].o.a.v = MetaVop(A[0].o.a.v);
  return VQ_view;
}

static vq_Type ReplaceCmd_VIIV (vq_Cell A[]) {
  A[0].o.a.v = ReplaceVop(A[0].o.a.v,A[1].o.a.i,A[2].o.a.i,A[3].o.a.v);
  return VQ_view;
}

#ifdef VQ_LOAD_H

static vq_Type AsMetaCmd_S (vq_Cell A[]) {
  A[0].o.a.v = AsMetaVop(A[0].o.a.s);
  return VQ_view;
}

static vq_Type OpenCmd_S (vq_Cell A[]) {
  A[0].o.a.v = OpenVop(A[0].o.a.s);
  return VQ_view;
}

#endif

#ifdef VQ_MUTABLE_H

static vq_Type MutableCmd_V (vq_Cell A[]) {
  A[0].o.a.v = MutableVop(A[0].o.a.v);
  return VQ_view;
}

#endif

#ifdef VQ_OPDEF_H

static vq_Type ColMapCmd_VV (vq_Cell A[]) {
  A[0].o.a.v = ColMapVop(A[0].o.a.v,A[1].o.a.v);
  return VQ_view;
}

static vq_Type PairCmd__V (vq_Cell A[]) {
  A[0].o.a.v = PairVop(A[0].o.a.v);
  return VQ_view;
}

static vq_Type PassCmd_V (vq_Cell A[]) {
  A[0].o.a.v = PassVop(A[0].o.a.v);
  return VQ_view;
}

static vq_Type PlusCmd__V (vq_Cell A[]) {
  A[0].o.a.v = PlusVop(A[0].o.a.v);
  return VQ_view;
}

static vq_Type RowMapCmd_VV (vq_Cell A[]) {
  A[0].o.a.v = RowMapVop(A[0].o.a.v,A[1].o.a.v);
  return VQ_view;
}

static vq_Type StepCmd_Iiiis (vq_Cell A[]) {
  A[0].o.a.v = StepVop(A[0].o.a.i,A[1].o.a.i,A[2].o.a.i,A[3].o.a.i,A[4].o.a.s);
  return VQ_view;
}

static vq_Type TypeCmd_V (vq_Cell A[]) {
  A[0].o.a.s = TypeVop(A[0].o.a.v);
  return VQ_string;
}

static vq_Type ViewCmd_Vv (vq_Cell A[]) {
  A[0].o.a.v = ViewVop(A[0].o.a.v,A[1].o.a.v);
  return VQ_view;
}

#endif

CmdDispatch f_vdispatch[] = {
  { "empty", "T:VII", EmptyCmd_VII },
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
  { "colmap", "V:VV", ColMapCmd_VV },
  { "pair", "V:_V", PairCmd__V },
  { "pass", "V:V", PassCmd_V },
  { "plus", "V:_V", PlusCmd__V },
  { "rowmap", "V:VV", RowMapCmd_VV },
  { "step", "V:Iiiis", StepCmd_Iiiis },
  { "type", "S:V", TypeCmd_V },
  { "view", "V:Vv", ViewCmd_Vv },
#endif
  {NULL,NULL,NULL}
};

#endif

/* end of generated code */
