/* xvec.h -=- Packed integer vector access extension */

  L I get_v0z(P xa, I xi) {
    S xp = String(xa);
    
    intPush(Work(xa), xp-xp);
    return 0;
  }

  L I get_v1u(P xa, I xi) {
    S xp = String(xa);
    S t = xp + (xi>>3);
    intPush(Work(xa), (*t >> (xi&7)) & 1);
    return 0;
  }

  L I get_v2u(P xa, I xi) {
    S xp = String(xa);
    S t = xp + (xi>>2);
    intPush(Work(xa), (*t >> 2*(xi&3)) & 3);
    return 0;
  }

  L I get_v3u(P xa, I xi) {
    S xp = String(xa);
    S t = xp + (xi>>1);
    intPush(Work(xa), (*t >> 4*(xi&1)) & 15);
    return 0;
  }

  L I get_v4i(P xa, I xi) {
    S xp = String(xa);
    S t = xp + xi;
    intPush(Work(xa), *t);
    return 0;
  }

  L I get_v5i(P xa, I xi) {
    S xp = String(xa) + 2*xi;
    H v;
    ((M) &v)[0] = xp[0];
    ((M) &v)[1] = xp[1];
    intPush(Work(xa), v);
    return 0;
  }

  L I get_v6i(P xa, I xi) {
    S xp = String(xa) + 4*xi;
    Q v;
    ((M) &v)[0] = xp[0];
    ((M) &v)[1] = xp[1];
    ((M) &v)[2] = xp[2];
    ((M) &v)[3] = xp[3];
    intPush(Work(xa), v);
    return 0;
  }

  L I get_v5r(P xa, I xi) {
    S xp = String(xa) + 2*xi;
    H v;
    ((M) &v)[1] = xp[0];
    ((M) &v)[0] = xp[1];
    intPush(Work(xa), v);
    return 0;
  }

  L I get_v6r(P xa, I xi) {
    S xp = String(xa) + 4*xi;
    Q v;
    ((M) &v)[3] = xp[0];
    ((M) &v)[2] = xp[1];
    ((M) &v)[1] = xp[2];
    ((M) &v)[0] = xp[3];
    intPush(Work(xa), v);
    return 0;
  }

L I vecExt(P p, I c) { P w = Work(p);
  extCode(w,"xv0z",get_v0z);
  extCode(w,"xv1u",get_v1u);
  extCode(w,"xv2u",get_v2u);
  extCode(w,"xv3u",get_v3u);
  extCode(w,"xv4i",get_v4i);
  extCode(w,"xv5i",get_v5i);
  extCode(w,"xv6i",get_v6i);
  extCode(w,"xv5r",get_v5r);
  extCode(w,"xv6r",get_v6r);
  return 0;
}
