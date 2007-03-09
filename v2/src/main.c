/* main.c -=- Vlerq as a standalone app */

#define NAME "Vq"

#include "thrive.h"
#include "sysdep.h"

/* use a stack of input streams to support nested evaluation of scripts */
L struct Source { M line, tokn; struct Source* prev; C data[1]; } *gSrc;

L M gTok; /* start of next token */

/* push string as next input */
L V myInput(S s, I n) {
  struct Source* src = (struct Source*) malloc(sizeof (struct Source) + n);
  src->line = memcpy(src->data, s, n);
  src->line[n] = 0;
  Z printf("input <%s>\n",src->line);
  src->tokn = gTok;
  src->prev = gSrc;
  gSrc = src;
  gTok = 0;
}

/* advance to next token from input stack */
L I myGets() {
  while (gTok == 0) {
    struct Source* src = gSrc;
    if (src == 0) return 0;
    gTok = src->line;
    if (*gTok != 0) {
      M eolptr = strchr(gTok,'\n');
      if (eolptr == 0) eolptr = gTok + strlen(gTok); else *eolptr++ = 0;
      Z printf("line <%s>\n",gTok);
      src->line = eolptr;
      if (*gTok == 'D' && whsp(gTok[1])) { *gTok = '#'; D *gTok = ' '; }
      if (*gTok == '\\' || *gTok == '#') { gTok = 0; continue; }
      break;
    }
    gSrc = src->prev;
    gTok = src->tokn;
    free(src);
  }
  return 1;
}

L V printVal(B b, I l) {
  if (isInt(b))
    printf("%ld",b.i);
  else if (isVec(b))
    if (--l < 0)
      printf("<0x%lx,%ld>",(U)b.p,b.i);
    else if (b.i < 0) {
      int i = 0;
      putchar('[');
      while (i < b.p[-1].i) {
	if (i > 0) putchar(' ');
	if (i >= 10) { printf("..#%ld",b.p[-1].i); break; }
	printVal(b.p[i++],l);
      }
      putchar(']');
    } else
      printVal(b.p[b.i],l);
  else if (strAt(b) == 0)
    printf("nil");
  else
    fwrite(strAt(b),lenAt(b),1,stdout);
}

/* Read a 4-byte big-endian int from current file pos */
L I read4b(FILE* fd) {
  I i = (fgetc(fd) & 0xFF) << 24;
  i |= (fgetc(fd) & 0xFF) << 16;
  i |= (fgetc(fd) & 0xFF) << 8;
  return i | (fgetc(fd) & 0xFF);
}

/* This program can be launched in different modes:
 *    embed   embedded script, leave args as is
 *    file    first arg is a script
 *    bang    as #! starting line (treated same as "file" mode)
 *    stdin   no args (or "-" first arg), prompt if input & output are ttys
 *
 * When running embedded, it is still possible to use the other modes:
 * if there is no "main.th", then continue to try file/bang mode as well.
 */

L I initScript(P w, I ac, C** av) {
  P p;
  char path [1024];
  S self = pFindExecutable(*av,path,sizeof path);
  I ok = 0;
  strPush(w,self,~0);

  if (ac > 1) {
    if (strcmp(av[1],"-") != 0) {
      strPush(w,av[1],~0);
      mmfExt(w,0);
      if (Wtopd.i >= -1) 
	myInput(strAt(Wtopd),lenAt(Wtopd));
      nilPush(w,-1);
    }
  }

  p = boxPush(w,ac);
  while (--ac >= 0) strPush(p,*av++,~0);

  if (self != 0 && *self != 0) {
    FILE *fd = fopen(self,"rb");
    if (fd != 0) {
      I ulen, zlen, mark = 0x74586953; /* tXiS */
      if (fseek(fd,-12,2) == 0 &&
	  ((ulen = read4b(fd)) == mark ||
           (fseek(fd,-ulen-20,1) == 0 && read4b(fd) == mark))) {
        ulen = read4b(fd);
        zlen = read4b(fd);
        if (fseek(fd,-zlen-12,1) == 0) {
	  I top = Wd.i;
	  M ubuf = (M)strPush(w,0,ulen);
	  if (ulen == zlen) 
	    ok = fread(ubuf,ulen,1,fd) == 1;
	  else {
	    M zbuf = (M)strPush(w,0,zlen);
	    ok = fread(zbuf,zlen,1,fd) == 1 && dezExt(w,0) == 0;
	  }
	  if (ok) 
	    myInput(ubuf,ulen);
	  Wd.i = top;
	}
      }
      fclose(fd);
    }
  }
  return isatty(0) && isatty(1);
}

int main(int ac, C** av) {
  P w = newWork();
  I clk = 0, prompt = initScript(w,ac,av);
  for (;;) {
    S s;
    I i, req = runWork(w,0);
    Z if (req != 0) printf("syst %ld top %ld\n",req,Length(w));
    switch (req) {
      case 0: /* (tok) ( - s ) */
	for (;;) {
	  if (gTok == 0 && !myGets()) {
	    C b[250];
	    if (prompt) {
	      printf(" %03ld> ", clk ? (getclicks()-clk+500)/1000 : VERSION);
	      fflush(stdout);
	    }
	    if (fgets(b,sizeof b,stdin) == 0) {
	      if (prompt) printf("\n");
	      s = 0;
	      break;
	    }
	    myInput(b,strlen(b));
	    if (prompt) clk = getclicks();
	    continue;
	  }
	  s = scanSym(&gTok);
	  if (s != 0 && *s != '#') break;
	}
	if (s == 0) break;
	Z printf("<%s>\n",s);
	strPush(w,s,~0);
	i = findSym(w);
	if (i >= 0) evalSym(w,i);
	continue;
      case 1: /* heval ( ss - ) */
	nilPush(w,-1);
	myInput(strAt(Wtopd),lenAt(Wtopd));
	nilPush(w,-1);
	continue;
      case 2: /* quit ( i - ) */
	req = Wtopd.i;
	break;
      case 3: /* .p ( b - ) */
	printVal(Wtopd,1);
	nilPush(w,-1);
	continue;
      /* case 4: readf, not implemented */
      case 5: /* musecs ( - n ) */
	intPush(w,getclicks());
	continue;
      case 6: /* getenv ( s - s ) */
	s = getenv(cString(w,-1));
	nilPush(w,-1);
	if (s != 0) strPush(w,s,~0); else nilPush(w,1);
	continue;
      case 7: /* fseek ( i i i - i ) */
	i = Wpopd.i;
	fseek((FILE*)Wsubd.i,Wtopd.i,i);
	i = (I)ftell((FILE*)Wsubd.i);
	nilPush(w,-2);
	intPush(w,i);
	continue;
      case 8: /* fread ( i s - i ) */
	i = fread((M)strAt(Wtopd),1,lenAt(Wtopd),(FILE*)Wsubd.i);
	nilPush(w,-2);
	intPush(w,i);
	continue;
      case 9: /* fwrite ( i s - i ) */
	i = fwrite(strAt(Wtopd),1,lenAt(Wtopd),(FILE*)Wsubd.i);
	nilPush(w,-2);
	intPush(w,i);
	continue;
      case 10: /* fopen ( s s - i ) */
	i = (I)fopen(cString(w,-2),cString(w,-1));
	/* setbuf((FILE*)i,0); */
	nilPush(w,-2);
	if (i != 0) intPush(w,i); else nilPush(w,1);
	continue;
      case 11: /* fclose ( i - ) */
	fclose((FILE*) Wpopd.i);
	continue;
    }
    endWork(w);
    free(gSrc);
    return req;
  }
}
