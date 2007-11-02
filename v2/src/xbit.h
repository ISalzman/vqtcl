/* xbit.h -=- Bit operation extension */

L const char bits[256] = {
  0,1,1,2,1,2,2,3, 1,2,2,3,2,3,3,4, 1,2,2,3,2,3,3,4, 2,3,3,4,3,4,4,5,
  1,2,2,3,2,3,3,4, 2,3,3,4,3,4,4,5, 2,3,3,4,3,4,4,5, 3,4,4,5,4,5,5,6,
  1,2,2,3,2,3,3,4, 2,3,3,4,3,4,4,5, 2,3,3,4,3,4,4,5, 3,4,4,5,4,5,5,6,
  2,3,3,4,3,4,4,5, 3,4,4,5,4,5,5,6, 3,4,4,5,4,5,5,6, 4,5,5,6,5,6,6,7,
  1,2,2,3,2,3,3,4, 2,3,3,4,3,4,4,5, 2,3,3,4,3,4,4,5, 3,4,4,5,4,5,5,6,
  2,3,3,4,3,4,4,5, 3,4,4,5,4,5,5,6, 3,4,4,5,4,5,5,6, 4,5,5,6,5,6,6,7,
  2,3,3,4,3,4,4,5, 3,4,4,5,4,5,5,6, 3,4,4,5,4,5,5,6, 4,5,5,6,5,6,6,7,
  3,4,4,5,4,5,5,6, 4,5,5,6,5,6,6,7, 4,5,5,6,5,6,6,7, 5,6,6,7,6,7,7,8,
};

/* This looks neat, but a 4-stage byte-wise table lookup might be faster */
L I topbit(UQ v) {
#define Vn(x) (v < (1<<x))
  return Vn(16) ? Vn( 8) ? Vn( 4) ? Vn( 2) ? Vn( 1) ? (I)v-1 : 1
					   : Vn( 3) ?  2 :  3
				  : Vn( 6) ? Vn( 5) ?  4 :  5
					   : Vn( 7) ?  6 :  7
			 : Vn(12) ? Vn(10) ? Vn( 9) ?  8 :  9
					   : Vn(11) ? 10 : 11
				  : Vn(14) ? Vn(13) ? 12 : 13
					   : Vn(15) ? 14 : 15
		: Vn(24) ? Vn(20) ? Vn(18) ? Vn(17) ? 16 : 17
					   : Vn(19) ? 18 : 19
				  : Vn(22) ? Vn(21) ? 20 : 21
					   : Vn(23) ? 22 : 23
			 : Vn(28) ? Vn(26) ? Vn(25) ? 24 : 25
					   : Vn(27) ? 26 : 27
				  : Vn(30) ? Vn(29) ? 28 : 29
					   : Vn(31) ? 30 : 31;
#undef Vn
}

L V emitbits(P p, UQ v) {
  S s = String(p);
  M q = (M)s + (p[Adr].i>>3);
  I r = 8-(p[Adr].i&7);
  I t = topbit(v), n = t+1;
  A(t >= 0);
  while (t >= r) { ++q; t -= r; r=8; }
  r -= t;
  while (n >= r) {
    *q++ |= (C)(v>>(n-r)); v &= (1<<(n-r))-1;
    n -= r; r=8;
  }
  *q |= (C)(v<<(r-n));
  p[Adr].i = ((q-s)<<3)+(8-r)+n;
  ++Length(p);
}

L I bitExt(P p, I c) {
  P w = Work(p), t = w+Wd.i-1;
  I i = 0;
  /*%renumber< case >%*/
  switch (c) {
    case 0: { /* count bits in bitmap ( b - n ) */
      S v = strAt(*t), e = v + t->i;
      while (v<e) { i += bits[(UC)*v++]; }
      t->i = i;
      break;
    }
    case 1: { /* locate n'th 1-bit ( b - n ) */
      S v0 = strAt(*t), v = v0, e = v + lenAt(*t);
      I x = t->i, r = -1;
      while (v<e) {
	i += bits[(UC)*v++];
	if (i > x) {
	  I f = 0x100;
	  r = 8*(v-v0); 
	  do { --r; if (v[-1] & (f>>=1)) --i; } while (i>x);
	  break;
	}
      }
      t->i = r;
      break;
    }
    case 2: { /* top bit in int ( n - n ) */
      t->i = topbit((UQ)t->i);
      break;
    }
    case 3: { /* convert bitmap to packed form ( b - b ) */
      S v = strAt(*t);
      I n = t->i, l = *v&1, c = 0;
      P r = newBuffer(w,0,(n+n/2)/8+1), r2;
      *((M)r) = l<<7;
      r[Adr].i = 1;
      while (i<n) {
	if (((v[i>>3]>>(i&7))&1) == l)
	  ++c;
	else { emitbits(r,c); c = 1; l ^= 1; }
	++i;
      }
      if (c) emitbits(r,c);
      t->p = r; /* put r on stack to prevent gc during next alloc */
      t->p = r2 = newBuffer(w,(S)r,(r[Adr].i+7)>>3);
      r2[Adr].i = r[Adr].i;
      Length(r2) = Length(r);
      break;
    }
    case 4: { /* get next packed entry ( b - n ) */
      P p = t->p;
      S s = String(p);
      M q = (M)s + (p[Adr].i>>3);
      I r = 8-(p[Adr].i&7);
      t->i = 0;
      for (;;) {
	++i;
	if (*q & (1<<(r-1))) break;
	if (--r <= 0) { if (++q >= s+p[Cnt].i) return 0; else r=8; }
      }
      while (i>=r) { t->i <<= r; t->i |= (*q++&((1<<r)-1)); i-=r; r=8; }
      t->i <<= i;
      t->i |= ((*q>>(r-i))&((1<<i)-1));
      p[Adr].i = ((q-s)<<3)+(8-r)+i;
      break;
    }
    case 5: { /* lowest and highest values in v32s ( v - i i ) */
      Q *v = (Q*) String(t->p), *e = v + Length(t->p), l = 0, h = 0;
      while (v < e) { Q x = *v++; if (x < l) l = x; if (x > h) h = x; }
      t->i = l;
      Wpushd.i = h; Wtopd.p = t->p;
      break;
    }
    case 6: { /* convert v32s to smaller ints ( s n - s ) */
      I n = Wpopd.i;
      if (0 <= n && n <= 5) { 
	I m = n == 0 ? 0 : lenAt(Wtopd);
	Q *v = (Q*) strAt(Wtopd), *e = v + m;
	P b = newBuffer(w,0,n<0?0:((m<<n)+14)>>4);
	switch (n) {
	  case 0: /* 0 bits */
	    break;
	  case 1: { /* 1 bit, 8/byte */
	    C *o = (C*)b;
	    while (v < e) { *o |= (*v++&1) << i; ++i; o += i>>3; i &= 7; }
	    break;
	  }
	  case 2: { /* 2 bits, 4/byte */
	    C *o = (C*)b;
	    while (v < e) { *o |= (*v++&3) << i; i += 2; o += i>>3; i &= 7; }
	    break;
	  }
	  case 3: { /* 4 bits, 2/byte */
	    C *o = (C*)b;
	    while (v < e) { *o |= (*v++&15) << i; i += 4; o += i>>3; i &= 7; }
	    break;
	  }
	  case 4: { /* 1-byte (char) */
	    C *o = (C*)b;
	    while (v < e) *o++ = (C)*v++;
	    break;
	  }
	  case 5: { /* 2-byte (short) */
	    H *o = (H*)b;
	    while (v < e) *o++ = (H)*v++;
	    break;
	  }
	}
	Wtopd.p = b;
	Length(b) = m;
      } else
	n = 6;
      Wtopd.i = n;
      break;
    }
  }
  /*%renumber<>%*/
  return 0;
}
