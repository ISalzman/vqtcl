/* xtcl.h -=- Tcl syntax scanner, adapted from "scratch" in critlib */

/* Character classes */
enum {
  eNormal       = 0x00,
  eSpace        = 0x01,
  eNewLine      = 0x02,
  eSemiColon    = 0x04,
  eSubstitute   = 0x08,
  eDoubleQuote  = 0x10,
  eCloseParen   = 0x20,
  eCloseBracket = 0x40,
  eBrace        = 0x80
};
enum {
  efrLiteral,
  efrEscaped,
  efrExtraAfterBrace,
  efrMissingBrace,
  efrExtraAfterQuote,
  efrMissingQuote,
  efrEof = -1
};

typedef struct {
  S from;
  S to;
} TclScanner;
typedef struct {
  TclScanner ts;
  P ws;
  S error;
} TclTokenizer;

L V te_AppendStack(P ws, P obj);
L P te_LitResult(P, char, S, S);
L V te_LitChar(P ws, char ch);
L V te_Literal(P ws, S from, S to);
L int te_Frame(P ws);
L V te_Command(P ws, int base);
L int te_Merge(P ws, int wbase);
L P te_Pop(P ws);
L V te_Scalar(P ws, S from, S to);
L V te_Array(P ws, S from, S to);

/* Look ahead to the next character */
L char ts_PeekChar(const TclScanner* ts) {
  return ts->from != ts->to ? *ts->from : 0;
}
/* Determine the character class */
L int ts_CharClass(char c) {
  return c == ' '  || c == '\t' ?            eSpace :
	 c == '\n' ?                         eNewLine :
	 c == ';' || c == 0 ?                eSemiColon :
	 c == '\\' || c == '$' || c == '[' ? eSubstitute :
	 c == '"' ? /*"*/                    eDoubleQuote :
	 c == ')' ?                          eCloseParen :
	 c == ']' ?                          eCloseBracket :
	 c == '{' || c == '}' ?              eBrace :
					     eNormal;
}
/* Return next character, or a null byte if at end */
L int ts_PeekClass(const TclScanner* ts) {
  return ts_CharClass(ts_PeekChar(ts));
}
/* Returns number of chars which can be replace by a space, or zero */
L int ts_EscapedNewLine(const TclScanner* ts) {
  int i = 2;
  if (ts->from + 1 >= ts->to || ts->from[0] != '\\' || ts->from[1] != '\n')
    return 0;
  while (ts->from + i < ts->to && ts_CharClass(ts->from[i]) == eSpace)
    ++i;
  return i;
}
/* Skip over whitespace and escaped newlines */
L V ts_SkipWhiteSpace(TclScanner* ts) {
  int i;
  for (;;) {
    while (ts_PeekClass(ts) & eSpace)
      ++ts->from;
    i = ts_EscapedNewLine(ts);
    if (i == 0)
      return;
    ts->from += i;
  }
}
/* Define a new scanner over a specified string */
L V ts_TclScanner (TclScanner* ts, S from, S to) {
  ts->from = from;
  ts->to = to;
}
L int isdiglet(char c, char e) {
  return ('0'<=c && c<='9') || ('A'<=c && c<=e) || ('a'<=c && c<=(e+32));
}
L char ts_ConvertBackslash(TclScanner* ts) {
  S start;
  char ch;
  if (ts->from == ts->to)
    return '\\';
  start = ts->from++;
  if (*start == 'x' && isdiglet(ts_PeekChar(ts),'F')) {
    char ch = 0;
    while (isdiglet(ts_PeekChar(ts),'F')) {
      ch = (char) (16 * ch + (*ts->from & 0x1F));
      if (*ts->from++ > '9')
	ch += 9;
    }
    return ch;
  }
  if (isdiglet(*start,'@')) {
    char ch = 0;
    while (isdiglet(ts_PeekChar(ts),'@'))
      ch = (char) (8 * ch + *ts->from++ - '0');
    return ch;
  }
  if (*start == '\n') {
    ts->from = start - 1;
    ts->from += ts_EscapedNewLine(ts);
    return ' ';
  }
  ch = *start;
  switch (ch) {
    case 'a': ch = 0x07; break;
    case 'b': ch = 0x08; break;
    case 'f': ch = 0x0C; break;
    case 'n': ch = 0x0A; break;
    case 'r': ch = 0x0D; break;
    case 't': ch = 0x09; break;
    case 'v': ch = 0x0B; break;
  }
  return ch;
}
#if 0
/* Return non-zero if next character is white space or newline */
L int ts_AtWhiteOrEol(const TclScanner* ts) {
  return ts_PeekClass(ts) & (eSpace | eNewLine);
}
/* Find the boundaries of the next list element */
L int ts_FindListElement(TclScanner* ts, S* begin, S* end) {
  int level = 0;
  int escaped = efrLiteral;
  while (ts_AtWhiteOrEol(ts))
    ++ts->from;
  if (ts->from == ts->to)
    return efrEof;
  switch (ts_PeekChar(ts)) {
    case '{':
      *begin = ++ts->from;
      while (ts->from != ts->to) {
	switch (*ts->from++) {
	  case '{':
	    ++level;
	    break;
	  case '}':
	    if (--level >= 0) break;
	    *end = ts->from - 1;
	    return ts_AtWhiteOrEol(ts) || ts->from == ts->to ? escaped :
		efrExtraAfterBrace; /* extra characters after close-brace */
	  case '\\':
	    if (ts->from != ts->to)
	      ++ts->from;
	    escaped = efrEscaped;
	}
      }
      return efrMissingBrace; /* missing closing brace in list element */
    case '"':
      *begin = ++ts->from;
      while (ts->from != ts->to) {
	switch (*ts->from++) {
	  case '"':
	    *end = ts->from - 1;
	    return ts_AtWhiteOrEol(ts) || ts->from == ts->to ? escaped :
		  efrExtraAfterQuote; /* extra characters after close-quote */
	  case '\\':
	    if (ts->from != ts->to)
	      ++ts->from;
	    escaped = efrEscaped;
	}
      }
      return efrMissingQuote; /* missing quote in list element */
    default:
      *begin = ts->from;
      do {
	if (*ts->from == '\\') {
	  if (++ts->from == ts->to) break;
	  escaped = efrEscaped;
	}
      } while (!ts_AtWhiteOrEol(ts) && ++ts->from != ts->to);
      *end = ts->from;
      return escaped;
  }
}
L int ts_ConvertListElement(TclScanner* ts, S begin, S end, char* buffer) {
  char* p = buffer;
  S save = ts->from;
  ts->from = begin;
  while (ts->from != end) {
    char c = *ts->from++;
    if (c == '\\' && ts->from != end)
      c = ts_ConvertBackslash(ts);
    *p++ = c;
  }
  ts->from = save;
  return p - buffer;
}
#endif
L V tt_Fail(TclTokenizer* tt, S error) {
  tt->error = error;
}
/* Fail and throw an error if next char is incorrect, else skip it */
L V tt_Require(TclTokenizer* tt, char ch, S error) {
  if (ts_PeekChar(&tt->ts) != ch)
    tt_Fail(tt, error);
  else
    ++tt->ts.from;
}
/* Define a new tokenizer over a specified string */
L V tt_TclTokenizer (TclTokenizer* tt, S from, S to, P ws) {
  ts_TclScanner(&tt->ts, from, to);
  tt->ws = ws;
  tt->error = 0;
}
L V tt_TokenizeScript(TclTokenizer* tt);
L V tt_TokenizeCommand(TclTokenizer* tt, int nested);
L int tt_TokenizeUntil(TclTokenizer* tt, int mask);
L int tt_TokenizeVariable(TclTokenizer* tt);
/* Tokenize all the commands */
L V tt_TokenizeScript(TclTokenizer* tt) {
  while (tt->ts.from != tt->ts.to) {
    tt_TokenizeCommand(tt, 0);
  }
}
/* Tokenize the next command, end at close bracket is nested */
L V tt_TokenizeCommand(TclTokenizer* tt, int nested) {
  int base, wbase, mask;
  const char *first, *end;
  for (;;) {
    ts_SkipWhiteSpace(&tt->ts);
    if (ts_PeekClass(&tt->ts) & (eNewLine | eSemiColon)) {
      if (tt->ts.from == tt->ts.to) return;
      ++tt->ts.from;
      continue;
    }
    if (ts_PeekChar(&tt->ts) != '#')
      break;
    while (++tt->ts.from != tt->ts.to) {
      /* this is non-conformant: no escaped backslashes */
      if (*tt->ts.from == '\n' && tt->ts.from[-1] != '\\')
	break;
    }
  }
  if (tt->ts.from == tt->ts.to) return;
  base = te_Frame(tt->ws);
  wbase = base;
  mask = eNewLine | eSemiColon;
  if (nested) mask |= eCloseBracket;
  first = tt->ts.from;
  while (tt->ts.from != tt->ts.to) {
    S start = tt->ts.from++;
    if (*start == '"') {
      tt_TokenizeUntil(tt, eDoubleQuote);
      tt_Require(tt, '"', "missing close-quote");
/*      if (tt->ts.from != tt->ts.to && !ts_AtWhiteOrEol(&tt->ts) && nested && *tt->ts.from != ']') */
/*        tt_Fail(tt, "extra characters after close-quote"); */
    }
    else if (*start == '{') {
      S token = tt->ts.from;
      int level = 0;
      for (;;) {
	while (ts_PeekClass(&tt->ts) == eNormal)
	  ++tt->ts.from;
	if (tt->ts.from == tt->ts.to) break;
	if (*tt->ts.from == '\\' && tt->ts.from + 1 != tt->ts.to) {
	  if (tt->ts.from[1] == '\n') {
	    if (tt->ts.from > token)
	      te_Literal(tt->ws, token, tt->ts.from);
	    tt->ts.from += ts_EscapedNewLine(&tt->ts);
	    te_LitChar(tt->ws, ' ');
	    token = tt->ts.from;
	  } else
	    ++tt->ts.from;
	} else if (*tt->ts.from == '{')
	  ++level;
	else if (*tt->ts.from == '}' && --level < 0)
	  break;
	++tt->ts.from;
      }
      if (tt->ts.from > token)
	te_Literal(tt->ws, token, tt->ts.from);
      tt_Require(tt, '}', "missing close-brace");
/*      if (tt->ts.from != tt->ts.to && !ts_AtWhiteOrEol(&tt->ts) && nested && *tt->ts.from != ']') */
/*        tt_Fail(tt, "extra characters after close-brace"); */
    } else {
      --tt->ts.from;
      tt_TokenizeUntil(tt, mask | eSpace);
    }
    end = tt->ts.from;
    ts_SkipWhiteSpace(&tt->ts);
    if (ts_PeekClass(&tt->ts) & mask)
      break;
    wbase = te_Merge(tt->ws, wbase);
  }
  te_Merge(tt->ws, wbase);
  te_Command(tt->ws, base);
}
/* Tokenize a number of tokens until specific characters are reached */
L int tt_TokenizeUntil(TclTokenizer* tt, int mask) {
  while (tt->ts.from != tt->ts.to) {
    S start = tt->ts.from;
    if (ts_CharClass(*tt->ts.from) & mask)
      return 1;
    switch (*tt->ts.from++) {
      case '$':
	if (!tt_TokenizeVariable(tt))
	  te_Literal(tt->ws, start, tt->ts.from);
	break;
      case '[':
	while (tt->ts.from != tt->ts.to && *tt->ts.from != ']')
	  tt_TokenizeCommand(tt, 1);
	tt_Require(tt, ']', "missing close-bracket");
	break;
      case '\\':
	--tt->ts.from;
	tt_Require(tt, '\\', "missing \\");
	te_LitChar(tt->ws, ts_ConvertBackslash(&tt->ts));
	if (start[1] == '\n' && (mask & eSpace)) return 0;
	break;
      default:
	while (tt->ts.from != tt->ts.to &&
	    (ts_CharClass(*tt->ts.from) & (mask | eSubstitute)) == 0)
	  ++tt->ts.from;
	te_Literal(tt->ws, start, tt->ts.from);
    }
  }
  return 1;
}
/* Tokenize veriable names, in all their variations */
L int tt_TokenizeVariable(TclTokenizer* tt) {
  if (ts_PeekChar(&tt->ts) == '{') {
    S start = ++tt->ts.from;
    while (tt->ts.from != tt->ts.to && *tt->ts.from++ != '}')
      ;
    --tt->ts.from;
    tt_Require(tt, '}', "missing close-brace for variable name");
    te_Scalar(tt->ws, start, tt->ts.from - 1);
  } else {
    S start = tt->ts.from;
    while (tt->ts.from != tt->ts.to) {
      if (!isdiglet(*tt->ts.from,'Z') && *tt->ts.from != '_') break;
      ++tt->ts.from;
    }
    if (ts_PeekChar(&tt->ts) != '(') {
      if (start == tt->ts.from)
	return 0; /* not a variable after all */
      te_Scalar(tt->ws, start, tt->ts.from);
    } else {
      S end = tt->ts.from;
      ++tt->ts.from;
      tt_TokenizeUntil(tt, eCloseParen);
      tt_Require(tt, ')', "missing )");
      te_Array(tt->ws, start, end);
    }
  }
  return 1;
}
L V te_AppendStack(P w, P obj) {
  P te = Wtopd.p;
  B b; b.p = obj; b.i = -1;
  if (Length(te) >= Limit(te))
    Wtopd.p = te = growBox(te);
  Push(te, b);
}
L P te_LitResult(P w, char type, S from, S to) {
  P r = newVector(w, 3);
  strPush(r, &type, 1);
  strPush(r, from, to-from);
  return r;
}
L V te_LitChar(P w, char ch) {
  P r = newVector(w, 2);
  strPush(r, "C", 1);
  intPush(r, ch);
  te_AppendStack(w, r);
}
L V te_Literal(P w, S from, S to) {
  if (from + 1 == to && (*from <= ' ' || *from > '~'))
    te_LitChar(w, *from);
  else
    te_AppendStack(w, te_LitResult(w, 'L', from, to));
}
L int te_Frame(P w) {
  return Length(Wtopd.p);
}
L V te_Command(P w, int base) {
  P te = Wtopd.p;
  int i, n = Length(te);
  P r = newVector(w, n-base+1);
  strPush(r, "X", 1);
  for (i = base; i < n; ++i)
    Push(r, te[i]);
  Length(te) = base;
  te_AppendStack(w, r);
}
L int te_Merge(P w, int base) {
  P te = Wtopd.p;
  int i, n = Length(te);
  if (n - base != 1) {
    P r = newVector(w, n-base+1);
    strPush(r, "M", 1);
    for (i = base; i < n; ++i)
      Push(r, te[i]);
    Length(te) = base;
    te_AppendStack(w, r);
  }
  return Length(te);
}
L P te_Pop(P w) {
  P te = Wtopd.p;
  return Length(te) > 0 ? te[--Length(te)].p : Wn.p;
}
L V te_Scalar(P w, S from, S to) {
  te_AppendStack(w, te_LitResult(w, 'S', from, to));
}
L V te_Array(P w, S from, S to) {
  P r = te_LitResult(w, 'A', from, to);
  B b; b.p = te_Pop(w); b.i = -1;
  Push(r,b);
  te_AppendStack(w, r);
}
L V te_TclScan(P w, S from, int len) {
  TclTokenizer tokenizer;
  te_AppendStack(w, newBuffer(w, "T", 1));
  tt_TclTokenizer(&tokenizer, from, from + len, w);
  tt_TokenizeScript(&tokenizer);
  if (tokenizer.error != 0)
    Wtopd.p = newBuffer(w, tokenizer.error, ~0);
}

L I tclExt(P p, I c) {
  P w = Work(p);
  switch (c) {
    case -1: /* init */
      return 0;
    case 0: { /* tclext ( s - b ) */
      boxPush(w, 0); /* careful, keep stack safe from gc */
      te_TclScan(w, strAt(Wsubd), lenAt(Wsubd));
      Wsubd = Wpopd;
      break;
    }
    default: A(0);
  }
  return 0;
}
