/* xops.h -=- Vector operation extension */

#define WVE while (v < e)

L I get_op0(P p, I c) {
  P w = Work(p), t = w + Wd.i - 1;
  P v = Wtopd.p;
  I m = Length(v);
  I i;
  /*%renumber< case >%*/
  switch (c) {
    case 0: { /* convert any vec to v32s ( v - b ) */
      F f = (F) v[Get].i;
      P xp = newBuffer(w,0,m<<2);
      Q *x = (Q*) xp;
      for (i = 0; i < m; ++i) {
	if (f(v,i)) {
	  D puts("xop0: oops, vec call returned nonzero!");
	  return 1;
	}
	*x++ = (Q) Wpopd.i;
      }
      Wtopd.p = xp; Wtopd.i = 32; Length(xp) = m;
      break;
    }
    case 1: { /* convert v32s with indices to bitmap ( n v - b ) */
      I n = Wsubd.i;
      P b = newBuffer(w,0,((n+127)>>7)<<4);
      C *x = (C*) b;
      Q *v = (Q*) strAt(Wtopd), *e = v + m;
      WVE { Q f = *v++; x[f>>3] |= 1 << (f&7); }
      Wpopd; Wtopd.p = b; Wtopd.i = 1; Length(b) = n;
      break;
    }
    case 2: { /* count bits in bitmap ( b - n ) */
      Q *v = (Q*) strAt(Wtopd), *e = v + ((lenAt(Wtopd) + 31) >> 5);
      i = 0;
      WVE { Q x = *v++;
	x = (x & 0x55555555) + ((x>>1) & 0x55555555);
	x = (x & 0x33333333) + ((x>>2) & 0x33333333);
	x = (x & 0x0F0F0F0F) + ((x>>4) & 0x0F0F0F0F);
	x = (x & 0x00FF00FF) + ((x>>8) & 0x00FF00FF);
	i += (char) x + (char) (x>>16);
      }
      Wtopd.p = Wz.p; Wtopd.i = i;
      break;
    }
    case 3: { /* convert bitmap to v32s index ( b n - v ) */
      I n = Wpopd.i;
      P xp = newBuffer(w,0,n<<2);
      Q *x = (Q*) xp;
      C *v = (C*) strAt(Wtopd), *e = v + ((lenAt(Wtopd) + 7) >> 3);
      i = 0;
      Wtopd.p = xp; Wtopd.i = 32;
      WVE { C k = *v++;
	if (k) {
	  C f = 1;
	  do { if (k & f) x[Length(xp)++] = i; ++i; f <<= 1; } while (f);
	} else
	  i += 8;
      }
      break;
    }
    case 4: { /* smallest value in v32s ( i v - i ) */
      Q *v = (Q*) strAt(Wtopd), *e = v + m;
      --t; Wpopd;
      WVE { Q x = *v++; if (x < t->i) t->i = x; }
      break;
    }
    case 5: { /* largest value in v32s ( i v - i ) */
      Q *v = (Q*) strAt(Wtopd), *e = v + m;
      --t; Wpopd;
      WVE { Q x = *v++; if (x > t->i) t->i = x; }
      break;
    }
    case 6: { /* convert v32s to cumulative ints ( n v - ) */
      Q *v = (Q*) strAt(Wtopd), *e = v + m;
      Wpopd; i = Wpopd.i;
      WVE { i += *v; *v++ = i; }
      break;
    }
    case 7: { /* fill v32s with constant ( n v - ) */
      Q *v = (Q*) strAt(Wtopd), *e = v + m;
      Wpopd; i = Wpopd.i;
      WVE *v++ = i;
      break;
    }
    case 8: { /* convert v32s to box ( v - b ) */
      Q *v = (Q*) strAt(Wtopd), *e = v + m;
      P b = newVector(w, m);
      Wtopd.p = b; Wtopd.i = -1; Length(b) = m;
      WVE b++->i = *v++;
      break;
    }
    case 9: { /* fill v32s with iota ( n - v ) */
      I n = Wtopd.i;
      P b = newBuffer(w,0,n<<2);
      Q *v = (Q*) b, *e = v + n;
      i = 0;
      WVE *v++ = i++;
      Wtopd.p = b; Wtopd.i = 32; Length(b) = n;
      break;
    }
    case 10: { /* flip bytes pairwise ( v - ) */
      C k, *v = (C*) strAt(Wtopd), *e = v + lenAt(Wpopd) * 2;
      WVE { k = *v++; *(v-1) = *v; *v++ = k; }
      break;
    }
    case 11: { /* flip bytes in v32s ( v - ) */
      C k, l, *v = (C*) strAt(Wtopd), *e = v + lenAt(Wpopd) * 4;
      WVE { k = *v++; l = *v++; *(v-1) = *v; *v++ = l; *(v-3) = *v; *v++ = k; }
      break;
    }
    case 12: { /* sum values in v32s ( v - i ) */
      Q *v = (Q*) strAt(Wtopd), *e = v + m, i = 0;
      WVE { i += *v++; }
      Wtopd.p = Wz.p; Wtopd.i = i;
      break;
    }
    case 13: { /* spread counts out to a group submap ( b n - b ) */
      I n = Wpopd.i;
      P b = newBuffer(w,0,n<<2);
      Q *x = (Q*) b;
      Q *v = (Q*) strAt(Wtopd), *e = v + Length(Wtopd.p);
      Q o = 0;
      WVE {
	Q k = *v++;
	if (k>0) { *x++ = o; i = 0; while (--k>0) *x++ = --i; }
	++o;
      }
      Wtopd.p = b; Wtopd.i = 32; Length(b) = n;
      break;
    }
    case 14: { /* binary search ( b k - i ) */
      I k = Wpopd.i;
      Q *v = (Q*) strAt(Wtopd);
      Q *l = v - 1, *u = v + Length(Wtopd.p);
      while (l + 1 != u) {
	Q *m = l + (u - l) / 2;
	if (*m < k) l = m; else u = m;
      }
      Wtopd.p = Wz.p; Wtopd.i = u - v;
      break;
    }
  }
  /*%renumber<>%*/
  return 0;
}

L I get_op1(P p, I c) {
  P w = Work(p);
  I x = Wpopd.i;
  P vp = Wpopd.p;
  Q *v = (Q*) String(vp), *e = v + Length(vp);
  /*%renumber< case >%*/
  switch (c) {
    case 0:  /* v+c */  WVE *v++ += x; break;
    case 1:  /* v*c */  WVE *v++ *= x; break;
    case 2:  /* v-c */  WVE *v++ -= x; break;
    case 3:  /* c-v */  WVE { *v = x - *v; ++v; } break;
    case 4:  /* v/c */  WVE *v++ /= x; break;
    case 5:  /* c/v */  WVE { *v = x / *v; ++v; } break;
    case 6:  /* v%c */  WVE *v++ %= x; break;
    case 7:  /* v&c */  WVE *v++ &= x; break;
    case 8:  /* v|c */  WVE *v++ |= x; break;
    case 9:  /* v^c */  WVE *v++ ^= x; break;
    case 10: /* v~c */  WVE *v++ &= ~x; break;
    case 11: /* c~v */  WVE { *v = x & ~*v; ++v; } break;
    case 12: /* v<<c */ x &= BPI-1; WVE *v++ <<= x; break;
    case 13: /* c<<v */ WVE { *v = x << (*v&(BPI-1)); ++v; } break;
    case 14: /* v>>c */ x &= BPI-1; WVE *v++ >>= x; break;
    case 15: /* c>>v */ WVE { *v = x >> (*v&(BPI-1)); ++v; } break;
  }
  /*%renumber<>%*/
  return 0;
}

L I get_op2(P p, I c) {
  P w = Work(p);
  P xp = Wpopd.p;
  Q *x = (Q*) String(xp);
  P vp = Wpopd.p;
  Q *v = (Q*) String(vp), *e = v + Length(vp);
  /*%renumber< case >%*/
  switch (c) {
    case 0:  /* v+v */  WVE *v++ += *x++; break;
    case 1:  /* v*v */  WVE *v++ *= *x++; break;
    case 2:  /* v-v */  WVE *v++ -= *x++; break;
    case 3:  /* v/v */  WVE *v++ /= *x++; break;
    case 4:  /* v%v */  WVE *v++ %= *x++; break;
    case 5:  /* v&v */  WVE *v++ &= *x++; break;
    case 6:  /* v|v */  WVE *v++ |= *x++; break;
    case 7:  /* v^v */  WVE *v++ ^= *x++; break;
    case 8:  /* v~v */  WVE *v++ &= ~*x++; break;
    case 9:  /* v<<v */ WVE *v++ <<= (*x++&(BPI-1)); break;
    case 10: /* v>>v */ WVE *v++ >>= (*x++&(BPI-1)); break;
  }
  /*%renumber<>%*/
  return 0;
}

L I get_op3(P p, I c) {
  P w = Work(p);
  I x = Wpopd.i;
  P vp = Wtopd.p;
  I m = Length(vp);
  Q *v = (Q*) String(vp), *e = v + m;
  P b = newBuffer(w,0,((m+127)>>7)<<4);
  C f = 1, *o = (C*) b;
  Wtopd.p = b; Wtopd.i = 1; Length(b) = m;
  /*%renumber< case >%*/
#define NEXT f <<= 1; if (f == 0) { ++f; ++o; }
  switch (c) {
    case 0:  /* v<c */  WVE { if (*v++ < x) *o|=f; NEXT }; break;
    case 1:  /* v==c */ WVE { if (*v++ == x) *o|=f; NEXT }; break;
    case 2:  /* v>c */  WVE { if (*v++ > x) *o|=f; NEXT }; break;
  }
#undef NEXT
  /*%renumber<>%*/
  return 0;
}

L I get_op4(P p, I c) {
  P w = Work(p);
  P xp = Wpopd.p;
  Q *x = (Q*) String(xp);
  Q *v = (Q*) strAt(Wtopd);
  I m = lenAt(Wtopd);
  Q *e = v + m;
  P b = newBuffer(w,0,((m+127)>>7)<<4);
  C f = 1, *o = (C*) b;
  Wtopd.p = b; Wtopd.i = 1; Length(b) = m;
  /*%renumber< case >%*/
#define NEXT f <<= 1; if (f == 0) { ++f; ++o; }
  switch (c) {
    case 0:  /* v<v */  WVE { if (*v++ < *x++) *o|=f; NEXT }; break;
    case 1:  /* v==v */ WVE { if (*v++ == *x++) *o|=f; NEXT }; break;
  }
#undef NEXT
  /*%renumber<>%*/
  return 0;
}

L I get_op5(P p, I c) {
  P w = Work(p);
  P xp = Wpopd.p;
  Q *x = (Q*) String(xp);
  Q *v = (Q*) strAt(Wtopd);
  Q *e = v + ((lenAt(Wtopd) + 31) >> 5); /*XXX assumes 32-bit ints */
  /*%renumber< case >%*/
  switch (c) {
    case 0:  /* v&v */  WVE *v++ &= *x++; break;
    case 1:  /* v|v */  WVE *v++ |= *x++; break;
    case 2:  /* v^v */  WVE *v++ ^= *x++; break;
    case 3:  /* v~v */  WVE *v++ &= ~*x++; break;
  }
  /*%renumber<>%*/
  return 0;
}

L I get_op6(P p, I c) {
  P w = Work(p);
  I x = Wpopd.i;
  P vp = Wtopd.p;
  Q *v = (Q*) String(vp), *e = v + Length(vp);
  I n = 0;
  /*%renumber< case >%*/
  switch (c) {
    case 0:  /* v<c */  WVE { if (*v++ < x) ++n; }; break;
    case 1:  /* v==c */ WVE { if (*v++ == x) ++n; }; break;
    case 2:  /* v>c */  WVE { if (*v++ > x) ++n; }; break;
  }
  /*%renumber<>%*/
  Wtopd.p = Wz.p; Wtopd.i = n;
  return 0;
}

L I get_op7(P p, I c) {
  P w = Work(p);
  P xp = Wpopd.p;
  Q *x = (Q*) String(xp);
  Q *v = (Q*) strAt(Wtopd);
  Q *e = v + lenAt(Wtopd);
  I n = 0;
  /*%renumber< case >%*/
  switch (c) {
    case 0:  /* v<v */  WVE { if (*v++ < *x++) ++n; }; break;
    case 1:  /* v==v */ WVE { if (*v++ == *x++) ++n; }; break;
  }
  /*%renumber<>%*/
  Wtopd.p = Wz.p; Wtopd.i = n;
  return 0;
}

#undef WVE

L I opsExt(P p, I c) {
  P w = Work(p);
  extCode(w,"xop0",get_op0);
  extCode(w,"xop1",get_op1);
  extCode(w,"xop2",get_op2);
  extCode(w,"xop3",get_op3);
  extCode(w,"xop4",get_op4);
  extCode(w,"xop5",get_op5);
  extCode(w,"xop6",get_op6);
  extCode(w,"xop7",get_op7);
  return 0;
}
