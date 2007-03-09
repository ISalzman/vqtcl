/* xstr.h -=- String operation extension */

#define isDigit(x) ('0' <= (x) && (x) <= '9')
#define toLower(x) ('A' <= (x) && (x) <= 'Z' ? (x)-'A'+'a' : (x))

#define NEXTL cl = lp < le ? *lp++ : 0
#define NEXTR cr = rp < re ? *rp++ : 0

L I dictcmp(B l, B r) {
  US lp = (US)strAt(l), rp = (US)strAt(r);
  US le = lp + lenAt(l), re = rp + lenAt(r);
  int cl, cr, d = 0, s = 0;
  NEXTL; NEXTR;
  while (d == 0) {
    if (isDigit(cl) && isDigit(cr)) {
      int z = 0;
      while (cl == '0' && lp < le && isDigit(*lp)) { NEXTL; ++z; }
      while (cr == '0' && rp < re && isDigit(*rp)) { NEXTR; --z; }
      if (s == 0) s = z;
      while (1) {
	if (d == 0) d = cl - cr;
	NEXTL; NEXTR;
	if (!isDigit(cr)) { if (isDigit(cl)) d = 1; break; }
	if (!isDigit(cl)) { d = -1; break; }
      }
    } else {
      d = toLower(cl) - toLower(cr);
      if (cl == 0 || cr == 0) break;
      if (s == 0) s = cl - cr;
      NEXTL; NEXTR;
    }
  }
  return d < 0 ? -2 : d > 0 ? 2 : s < 0 ? -1 : s > 0 ? 1 : 0;
}

#undef NEXTL
#undef NEXTR

L I strExt(P p, I c) {
  P w = Work(p);
  switch (c) {
    case -1: /* init */
      return 0;
    case 0: { /* dictcmp ( s s - i) */
      I f = dictcmp(Wsubd,Wtopd);
      Wpopd;
      Wtopd.p = Wz.p; Wtopd.i = f;
      break;
    }
    default: A(0);
  }
  return 0;
}
