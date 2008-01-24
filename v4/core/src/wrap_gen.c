/* wrap_gen.c - generated code, do not edit */

#include <stdlib.h>

#include "intern.h"
#include "wrap_gen.h"
  
ItemTypes BlockedCmd_V (Item_p a) {
  a[0].v = BlockedView(a[0].v);
  return IT_view;
}

ItemTypes CloneCmd_V (Item_p a) {
  a[0].v = CloneView(a[0].v);
  return IT_view;
}

ItemTypes CoerceCmd_OS (Item_p a) {
  a[0].c = CoerceCmd(a[0].o, a[1].s);
  return IT_column;
}

ItemTypes ColMapCmd_Vn (Item_p a) {
  a[0].v = ColMapView(a[0].v, a[1].c);
  return IT_view;
}

ItemTypes ColOmitCmd_Vn (Item_p a) {
  a[0].v = ColOmitView(a[0].v, a[1].c);
  return IT_view;
}

ItemTypes CompareCmd_VV (Item_p a) {
  a[0].i = ViewCompare(a[0].v, a[1].v);
  return IT_int;
}

ItemTypes CompatCmd_VV (Item_p a) {
  a[0].i = ViewCompat(a[0].v, a[1].v);
  return IT_int;
}

ItemTypes ConcatCmd_VV (Item_p a) {
  a[0].v = ConcatView(a[0].v, a[1].v);
  return IT_view;
}

ItemTypes CountsColCmd_C (Item_p a) {
  a[0].c = NewCountsColumn(a[0].c);
  return IT_column;
}

ItemTypes CountViewCmd_I (Item_p a) {
  a[0].v = NoColumnView(a[0].i);
  return IT_view;
}

ItemTypes FirstCmd_VI (Item_p a) {
  a[0].v = FirstView(a[0].v, a[1].i);
  return IT_view;
}

ItemTypes GetColCmd_VN (Item_p a) {
  a[0].c = ViewCol(a[0].v, a[1].i);
  return IT_column;
}

ItemTypes GetInfoCmd_VVI (Item_p a) {
  a[0].c = GetHashInfo(a[0].v, a[1].v, a[2].i);
  return IT_column;
}

ItemTypes GroupCmd_VnS (Item_p a) {
  a[0].v = GroupCol(a[0].v, a[1].c, a[2].s);
  return IT_view;
}

ItemTypes GroupedCmd_ViiS (Item_p a) {
  a[0].v = GroupedView(a[0].v, a[1].c, a[2].c, a[3].s);
  return IT_view;
}

ItemTypes HashFindCmd_VIViii (Item_p a) {
  a[0].i = HashDoFind(a[0].v, a[1].i, a[2].v, a[3].c, a[4].c, a[5].c);
  return IT_int;
}

ItemTypes HashViewCmd_V (Item_p a) {
  a[0].c = HashValues(a[0].v);
  return IT_column;
}

ItemTypes IjoinCmd_VV (Item_p a) {
  a[0].v = IjoinView(a[0].v, a[1].v);
  return IT_view;
}

ItemTypes IotaCmd_I (Item_p a) {
  a[0].c = NewIotaColumn(a[0].i);
  return IT_column;
}

ItemTypes IsectMapCmd_VV (Item_p a) {
  a[0].c = IntersectMap(a[0].v, a[1].v);
  return IT_column;
}

ItemTypes JoinCmd_VVS (Item_p a) {
  a[0].v = JoinView(a[0].v, a[1].v, a[2].s);
  return IT_view;
}

ItemTypes LastCmd_VI (Item_p a) {
  a[0].v = LastView(a[0].v, a[1].i);
  return IT_view;
}

ItemTypes MdefCmd_O (Item_p a) {
  a[0].v = ObjAsMetaView(a[0].o);
  return IT_view;
}

ItemTypes MdescCmd_S (Item_p a) {
  a[0].v = DescToMeta(a[0].s);
  return IT_view;
}

ItemTypes MetaCmd_V (Item_p a) {
  a[0].v = V_Meta(a[0].v);
  return IT_view;
}

ItemTypes OmitMapCmd_iI (Item_p a) {
  a[0].c = OmitColumn(a[0].c, a[1].i);
  return IT_column;
}

ItemTypes OneColCmd_VN (Item_p a) {
  a[0].v = OneColView(a[0].v, a[1].i);
  return IT_view;
}

ItemTypes PairCmd_VV (Item_p a) {
  a[0].v = PairView(a[0].v, a[1].v);
  return IT_view;
}

ItemTypes RemapSubCmd_ViII (Item_p a) {
  a[0].v = RemapSubview(a[0].v, a[1].c, a[2].i, a[3].i);
  return IT_view;
}

ItemTypes ReplaceCmd_VIIV (Item_p a) {
  a[0].v = ViewReplace(a[0].v, a[1].i, a[2].i, a[3].v);
  return IT_view;
}

ItemTypes RowCmpCmd_VIVI (Item_p a) {
  a[0].i = RowCompare(a[0].v, a[1].i, a[2].v, a[3].i);
  return IT_int;
}

ItemTypes RowEqCmd_VIVI (Item_p a) {
  a[0].i = RowEqual(a[0].v, a[1].i, a[2].v, a[3].i);
  return IT_int;
}

ItemTypes RowHashCmd_VI (Item_p a) {
  a[0].i = RowHash(a[0].v, a[1].i);
  return IT_int;
}

ItemTypes SizeCmd_V (Item_p a) {
  a[0].i = ViewSize(a[0].v);
  return IT_int;
}

ItemTypes SortMapCmd_V (Item_p a) {
  a[0].c = SortMap(a[0].v);
  return IT_column;
}

ItemTypes StepCmd_VIIII (Item_p a) {
  a[0].v = StepView(a[0].v, a[1].i, a[2].i, a[3].i, a[4].i);
  return IT_view;
}

ItemTypes StrLookupCmd_Ss (Item_p a) {
  a[0].i = StringLookup(a[0].s, a[1].c);
  return IT_int;
}

ItemTypes TagCmd_VS (Item_p a) {
  a[0].v = TagView(a[0].v, a[1].s);
  return IT_view;
}

ItemTypes TakeCmd_VI (Item_p a) {
  a[0].v = TakeView(a[0].v, a[1].i);
  return IT_view;
}

ItemTypes UngroupCmd_VN (Item_p a) {
  a[0].v = UngroupView(a[0].v, a[1].i);
  return IT_view;
}

ItemTypes UniqueMapCmd_V (Item_p a) {
  a[0].c = UniqMap(a[0].v);
  return IT_column;
}

ItemTypes ViewAsColCmd_V (Item_p a) {
  a[0].c = ViewAsCol(a[0].v);
  return IT_column;
}

ItemTypes WidthCmd_V (Item_p a) {
  a[0].i = ViewWidth(a[0].v);
  return IT_int;
}

/* : append ( VV-V ) over size swap insert ; */
ItemTypes AppendCmd_VV (Item_p a) {
  ItemTypes t;
  /* over 0 */ a[2] = a[0];
  t = SizeCmd_V(a+2);
  /* swap 1 */ a[3] = a[1]; a[1] = a[2]; a[2] = a[3];
  t = InsertCmd_VIV(a);
  return IT_view;
}

/* : colconv ( C-C )   ; */
ItemTypes ColConvCmd_C (Item_p a) {
  return IT_column;
}

/* : counts ( VN-C ) getcol countscol ; */
ItemTypes CountsCmd_VN (Item_p a) {
  ItemTypes t;
  t = GetColCmd_VN(a);
  t = CountsColCmd_C(a);
  return IT_column;
}

/* : delete ( VII-V ) 0 countview replace ; */
ItemTypes DeleteCmd_VII (Item_p a) {
  ItemTypes t;
  a[3].i = 0;
  t = CountViewCmd_I(a+3);
  t = ReplaceCmd_VIIV(a);
  return IT_view;
}

/* : except ( VV-V ) over swap exceptmap remap ; */
ItemTypes ExceptCmd_VV (Item_p a) {
  ItemTypes t;
  /* over 0 */ a[2] = a[0];
  /* swap 1 */ a[3] = a[1]; a[1] = a[2]; a[2] = a[3];
  t = ExceptMapCmd_VV(a+1);
  t = RemapCmd_Vi(a);
  return IT_view;
}

/* : exceptmap ( VV-C ) over swap isectmap swap size omitmap ; */
ItemTypes ExceptMapCmd_VV (Item_p a) {
  ItemTypes t;
  /* over 0 */ a[2] = a[0];
  /* swap 1 */ a[3] = a[1]; a[1] = a[2]; a[2] = a[3];
  t = IsectMapCmd_VV(a+1);
  /* swap 0 */ a[2] = a[0]; a[0] = a[1]; a[1] = a[2];
  t = SizeCmd_V(a+1);
  t = OmitMapCmd_iI(a);
  return IT_column;
}

/* : insert ( VIV-V ) 0 swap replace ; */
ItemTypes InsertCmd_VIV (Item_p a) {
  ItemTypes t;
  a[3].i = 0;
  /* swap 2 */ a[4] = a[2]; a[2] = a[3]; a[3] = a[4];
  t = ReplaceCmd_VIIV(a);
  return IT_view;
}

/* : intersect ( VV-V ) over swap isectmap remap ; */
ItemTypes IntersectCmd_VV (Item_p a) {
  ItemTypes t;
  /* over 0 */ a[2] = a[0];
  /* swap 1 */ a[3] = a[1]; a[1] = a[2]; a[2] = a[3];
  t = IsectMapCmd_VV(a+1);
  t = RemapCmd_Vi(a);
  return IT_view;
}

/* : namecol ( V-V ) meta 0 onecol ; */
ItemTypes NameColCmd_V (Item_p a) {
  ItemTypes t;
  t = MetaCmd_V(a);
  a[1].i = 0;
  t = OneColCmd_VN(a);
  return IT_view;
}

/* : names ( V-C ) meta 0 getcol ; */
ItemTypes NamesCmd_V (Item_p a) {
  ItemTypes t;
  t = MetaCmd_V(a);
  a[1].i = 0;
  t = GetColCmd_VN(a);
  return IT_column;
}

/* : product ( VV-V ) over over size spread rrot swap size repeat pair ; */
ItemTypes ProductCmd_VV (Item_p a) {
  ItemTypes t;
  /* over 0 */ a[2] = a[0];
  /* over 1 */ a[3] = a[1];
  t = SizeCmd_V(a+3);
  t = SpreadCmd_VI(a+2);
  /* rrot 0 */ a[3] = a[2]; a[2] = a[1]; a[1] = a[0]; a[0] = a[3];
  /* swap 1 */ a[3] = a[1]; a[1] = a[2]; a[2] = a[3];
  t = SizeCmd_V(a+2);
  t = RepeatCmd_VI(a+1);
  t = PairCmd_VV(a);
  return IT_view;
}

/* : remap ( Vi-V ) 0 -1 remapsub ; */
ItemTypes RemapCmd_Vi (Item_p a) {
  ItemTypes t;
  a[2].i = 0;
  a[3].i = -1;
  t = RemapSubCmd_ViII(a);
  return IT_view;
}

/* : repeat ( VI-V ) over size imul 0 1 1 step ; */
ItemTypes RepeatCmd_VI (Item_p a) {
  ItemTypes t;
  /* over 0 */ a[2] = a[0];
  t = SizeCmd_V(a+2);
  /* imul 1 */ a[1].i *=  a[2].i;
  a[2].i = 0;
  a[3].i = 1;
  a[4].i = 1;
  t = StepCmd_VIIII(a);
  return IT_view;
}

/* : reverse ( V-V ) dup size -1 1 -1 step ; */
ItemTypes ReverseCmd_V (Item_p a) {
  ItemTypes t;
  /* dup 0 */ a[1] = a[0];
  t = SizeCmd_V(a+1);
  a[2].i = -1;
  a[3].i = 1;
  a[4].i = -1;
  t = StepCmd_VIIII(a);
  return IT_view;
}

/* : slice ( VIII-V ) rrot 1 swap step ; */
ItemTypes SliceCmd_VIII (Item_p a) {
  ItemTypes t;
  /* rrot 1 */ a[4] = a[3]; a[3] = a[2]; a[2] = a[1]; a[1] = a[4];
  a[4].i = 1;
  /* swap 3 */ a[5] = a[3]; a[3] = a[4]; a[4] = a[5];
  t = StepCmd_VIIII(a);
  return IT_view;
}

/* : sort ( V-V ) dup sortmap remap ; */
ItemTypes SortCmd_V (Item_p a) {
  ItemTypes t;
  /* dup 0 */ a[1] = a[0];
  t = SortMapCmd_V(a+1);
  t = RemapCmd_Vi(a);
  return IT_view;
}

/* : spread ( VI-V ) over size 0 rot 1 step ; */
ItemTypes SpreadCmd_VI (Item_p a) {
  ItemTypes t;
  /* over 0 */ a[2] = a[0];
  t = SizeCmd_V(a+2);
  a[3].i = 0;
  /* rot 1 */ a[4] = a[1]; a[1] = a[2]; a[2] = a[3]; a[3] = a[4];
  a[4].i = 1;
  t = StepCmd_VIIII(a);
  return IT_view;
}

/* : types ( V-C ) meta 1 getcol ; */
ItemTypes TypesCmd_V (Item_p a) {
  ItemTypes t;
  t = MetaCmd_V(a);
  a[1].i = 1;
  t = GetColCmd_VN(a);
  return IT_column;
}

/* : union ( VV-V ) over except concat ; */
ItemTypes UnionCmd_VV (Item_p a) {
  ItemTypes t;
  /* over 0 */ a[2] = a[0];
  t = ExceptCmd_VV(a+1);
  t = ConcatCmd_VV(a);
  return IT_view;
}

/* : unionmap ( VV-C ) swap exceptmap ; */
ItemTypes UnionMapCmd_VV (Item_p a) {
  ItemTypes t;
  /* swap 0 */ a[2] = a[0]; a[0] = a[1]; a[1] = a[2];
  t = ExceptMapCmd_VV(a);
  return IT_column;
}

/* : unique ( V-V ) dup uniquemap remap ; */
ItemTypes UniqueCmd_V (Item_p a) {
  ItemTypes t;
  /* dup 0 */ a[1] = a[0];
  t = UniqueMapCmd_V(a+1);
  t = RemapCmd_Vi(a);
  return IT_view;
}

/* : viewconv ( V-V )   ; */
ItemTypes ViewConvCmd_V (Item_p a) {
  return IT_view;
}

/* end of generated code */
