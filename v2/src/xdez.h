/* xdez.h -=- Lossless decompression (Zlib's inflate alhorithm) */

/* This source code was derived from zlib, to which this license applies:
 *
 *  zlib.h -- interface of the 'zlib' general purpose compression library
 *  version 1.1.4, March 11th, 2002
 *
 *  Copyright (C) 1995-2002 Jean-loup Gailly and Mark Adler
 *
 *  This software is provided 'as-is', without any express or implied
 *  warranty.  In no event will the authors be held liable for any damages
 *  arising from the use of this software.
 *
 *  Permission is granted to anyone to use this software for any purpose,
 *  including commercial applications, and to alter it and redistribute it
 *  freely, subject to the following restrictions:
 *
 *  1. The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would be
 *     appreciated but is not required.
 *  2. Altered source versions must be plainly marked as such, and must not be
 *     misrepresented as being the original software.
 *  3. This notice may not be removed or altered from any source distribution.
 *
 *  Jean-loup Gailly        Mark Adler
 *  jloup@gzip.org          madler@alumni.caltech.edu
 *
 *  The data format used by the zlib library is described by RFCs (Request for
 *  Comments) 1950 to 1952 in the files ftp://ds.internic.net/rfc/rfc1950.txt
 *  (zlib format), rfc1951.txt (deflate format) and rfc1952.txt (gzip format).
 */

typedef struct {
  I fill, bits;
  S ibuf;
  M obuf;
  P self;
} DEZ;

L I needbits(DEZ* z, I n) {
  A(n > 0);
  A(n <= 32);
  while (n > z->fill) {
    /* printf("NB n %d c %02x\n",n,*z->ibuf&0xff); */
    z->bits |= (*z->ibuf++ & 0xff) << z->fill;
    z->fill += 8;
  }
  return z->bits & ((1<<n)-1);
}
L I takebits(DEZ* z, I n) {
  I b = needbits(z,n);
  z->bits >>= n;
  z->fill -= n;
  return b;
}
L I decode(DEZ* z, I* v) {
  I n = *v;
  return v[1+(1<<n)+takebits(z,v[1+needbits(z,n)])];
}
L V decotree(DEZ* z, I* blens, I* ldeco, I n) {
  I k = 0;
  while (k < n) {
    I i, x = decode(z,ldeco);
    switch (x) {
      default:
	blens[k] = x;
	x = 1;
	break;
      case 16:
	x = 3+takebits(z,2);
	for (i = 0; i < x; ++i) blens[k+i] = blens[k-1];
	break;
      case 17:
	x = 3+takebits(z,3);
	for (i = 0; i < x; ++i) blens[k+i] = 0;
	break;
      case 18:
	x = 11+takebits(z,7);
	for (i = 0; i < x; ++i) blens[k+i] = 0;
	break;
    }
    k += x;
  }
}
L I decolen(DEZ* z, I x) {
  L const C e [] = { 0,0,1,2,3,4,5,0 };
  L const I o [] = { 3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,
		     35,43,51,59,67,83,99,115,131,163,195,227,258 };
  I b = e[x/4];
  if (b != 0) b = takebits(z,b);
  return o[x]+b;
}
L I decodist(DEZ* z, I x) {
  L const C e [] = { 0,0,1,2,3,4,5,6,7,8,9,10,11,12,13 };
  L const I o [] = { 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,
		     769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577 };
  I b = e[x/2];
  if (b != 0) b = takebits(z,b);
  return o[x]+b;
}
L V decodata(DEZ* z, I* ldeco, I* ddeco) {
  for (;;) {
    I x = decode(z,ldeco);
    if (x < 256)
      *z->obuf++ = x;
    else if (x == 256)
      break;
    else {
      I i = decolen(z,x-257);
      S d = z->obuf - decodist(z,decode(z,ddeco));
      while (--i >= 0) *z->obuf++ = *d++; }
  }
}
L I bitflip(I v, I b) {
  I i, o = 0;
  for (i = 0; i < b; ++i) if (v & (1<<i)) o |= 1 << (b-i-1);
  return o;
}
L I* canonhuff(DEZ* z, I* blens, I maxcode, I maxbits) {
  I i, code = 0, max = 0, limit = 0, count;
  I bcounts [26], nextcode [26];
  P tbl;
  I *dat, *bcodes = (I*) calloc(maxcode,sizeof(I));
  for (i = 0; i <= maxbits; ++i) bcounts[i] = 0;
  for (i = 0; i < maxcode; ++i) ++bcounts[blens[i]];
  *bcounts = 0;
  for (i = 1; i <= maxbits; ++i)
    nextcode[i] = code = (code + bcounts[i-1]) << 1;
  for (i = 0; i < maxcode; ++i) {
    I len = blens[i];
    if (len) bcodes[i] = nextcode[len]++;
  }
  for (i = 1; i <= maxbits; ++i)
    if (bcounts[i]) max = i;
  for (i = 0; i < maxcode; ++i)
    if (bcodes[i] >= limit) limit = bcodes[i]+1;
  count = 1<<max;
  tbl = newBuffer(Work(z->self),0,(1+count+limit)*sizeof(I));
  dat = (I*) String(tbl);
  *dat = max;
  for (i = 0; i < maxcode; ++i) {
    I len = blens[i];
    if (len) {
      I j;
      I rev = bitflip(bcodes[i],len);
      for (j = 0; j < (count>>len); ++j)
	dat[1+rev+(j<<len)] = len;
      dat[1+count+rev] = i;
    }
  }
  free(bcodes);
  return dat;
}
L V infblocks(DEZ* z) {
  L const I border [] = { 16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 };
  I last, blens [1+258+31+31], *ldeco = 0, *ddeco = 0;

  do {
    last = takebits(z,1);
    switch (takebits(z,2)) {
      case 0: {
        I n;
	takebits(z,z->fill&7);
	n = takebits(z,16);
	takebits(z,16);
	while (--n >= 0)
	  *z->obuf++ = *z->ibuf++;
	continue;
      }
      case 1: /* put canonhuff pointers in gc-checked cells to avoid cleanup */
	if (z->self[Adr].i != 123) {
	  I i = 0;
	  z->self[Adr].i = 123;
	  while (i < 144) blens[i++] = 8;
	  while (i < 256) blens[i++] = 9;
	  while (i < 280) blens[i++] = 7;
	  while (i < 288) blens[i++] = 8;
	  z->self[Adr].p = (P) canonhuff(z,blens,288,9);
	  for (i = 0; i < 32; i++) blens[i] = 5;
	  z->self[Dat].p = (P) canonhuff(z,blens,32,5);
	}
	ldeco = (I*) z->self[Adr].p;
	ddeco = (I*) z->self[Dat].p;
	break;
      case 2: {
	I hlit = takebits(z,5);
	I hdist = takebits(z,5);
	I hclen = takebits(z,4);
	I i = 0;
	while (i < hclen+4) blens[border[i++]] = takebits(z,3);
	while (i < 19) blens[border[i++]] = 0;
	decotree(z,blens,canonhuff(z,blens,19,7),258+hlit+hdist);
	ldeco = canonhuff(z,blens,257+hlit,25);
	z->self[Tag].p = (P) ldeco; /* so next canonhuff will not gc it (!) */
	ddeco = canonhuff(z,blens+257+hlit,1+hdist,25);
	break;
      }
      default:
	continue;
    }
    decodata(z,ldeco,ddeco);
  } while (!last);
}
L I dezExt(P box, I cmd) {
  P w = Work(box);
  I skip = 0;
  switch (cmd) {
    case -1: /* init */
      return 0;
    case 0: /* skip header */
      skip = 2;
    case 1: { /* inflate */
      DEZ zbuf;
      zbuf.fill = zbuf.bits = 0;
      zbuf.ibuf = String(Wtopd.p) + skip;
      --Length(w);
      zbuf.obuf = (M) String(Wtopd.p);
      zbuf.self = box;
      infblocks(&zbuf);
      box[Adr] = box[Dat] = box[Tag] = Wz;
      Length(Wtopd.p) = zbuf.obuf - String(Wtopd.p);
      break;
    }
    default: A(0);
  }
  return 0;
}
