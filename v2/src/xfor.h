/* xfor.h -=- General-purpose iterator extension */

/*
 *  This getter implements various iteration modes across views and boxes.
 *
 *  The command specifies what type of iterator to push:
 *    0000:  push current item as is
 *    1000:  push a v32 index list entry
 *    2000:  push a bitmap entry (only iterate over the 1's)
 *
 *  It also determines what to do with the custom function:
 *     000:  call it
 *     100:  ignore it
 *
 *  And lastly, it specifies how to clean up:
 *	 0:  no value left on stack
 *	 1:  stop iteration when true
 *	 2:  count how many times non-zero is returned
 *	 3:  set a bit in the bitmap if true
 *	 4:  add current index to vec if true
 *	 5:  add current index to v32 if true
 *	 6:  append top of stack to vec
 *	 7:  append top of stack int to v32
 *
 *  The actual command code is constructed by adding the above choices.
 *
 *  This code expects certain values on the return stack:
 *    Wp    : loop wrapper (this getter cannot be used via "exec")
 *    Wr[0] : current element (set up at -1 initially)
 *    Wr[1] : output object (p) and limit value (i)
 *    Wr[2] : custom function (may be a primitive)
 */

L I forExt(P box, I cmd) {
  P w = Work(box);
  I r = 0;
  if (cmd >= 0) {
    B* rp = w + Wr.i;
    if (rp->i >= 0) {
      /* cleanup after previous iteration */
      /*%renumber< case >%*/
      switch (cmd%100) {
	case 0: /* no cleanup */
	  break;
	case 1: /* stop iteration when true */
	  if (Wpopd.i) return 0;
	  break;
	case 2: /* count if true on stack */
	  if (Wpopd.i) ++Length(rp[1].p);
	  break;
	case 3: /* set bit in bitmap if true on stack */
	  if (Wpopd.i) {
	    P o = rp[1].p;
	    int b = rp->i;
	    ++Length(o);
	    ((M)String(o))[b>>3] |= 1 << (b&7);
	  }
	  break;
	case 4: /* append curr index to vec if true on stack */
	  if (Wpopd.i) {
	    P o = rp[1].p;
	    Q n = Length(o)++;
	    o[n].p = Wz.p;
	    o[n].i = rp[0].i;
	  }
	  break;
	case 5: /* append curr index to v32 if true on stack */
	  if (Wpopd.i) {
	    P o = rp[1].p;
	    ((Q*)String(o))[Length(o)++] = rp->i;
	  }
	  break;
	case 6: { /* append top of stack to vec */
	  P o = rp[1].p;
	  o[Length(o)++] = Wpopd;
	  break;
	}
	case 7: { /* append top of stack int to v32 */
	  P o = rp[1].p;
	  ((Q*)String(o))[Length(o)++] = Wpopd.i;
	  break;
	}
	default: A(0);
      }
      /*%renumber<>%*/
    }
    while (++rp->i < rp[1].i) {
      /* there are different ways to push the current item on the stack */
      /*%renumber< case >%*/
      switch (cmd/1000) {
	case 0: /* push item as is */
	  Wpushd = rp[0];
	  break;
	case 1: /* push v32 indexed version */
	  Wpushd.p = rp->p[Adr].p; Wtopd.i = ((Q*)strAt(rp[0]))[rp->i];
	  break;
	case 2: { /* skip 0's in bitmap, only iterate over the 1's */
	  int b = rp->i;
	  if (!(String(rp[1].p)[b>>3] & (1 << (b&7)))) continue;
	  Wpushd.p = rp->p[Adr].p; Wtopd.i = b;
	  break;
	}
	default: A(0);
      }
      /*%renumber<>%*/
      /* call function with curr item and arrange to be called again */
      --Wp.i;
      if ((cmd/100)%10 == 0) {
	/* may trigger a trampoline request */
	r = ((F)Getter(rp[2].p))(rp[2].p,rp[2].i);
      }
      break;
    }
  }
  return r;
}
