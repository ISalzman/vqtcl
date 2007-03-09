%token SYM INT STR FLT VAR OPS _ALT_
%token _OPS_ LE GE NE EQ AND OR

%left OR
%left AND
%left EQ NE
%left '<' '>' LE GE
%left '+' '-'
%left '*' '/'
%left '!' NEG       # negation--unary minus

%%

go: ex		  	{ Pass 1 }
  ;
ex: STR       	  	{ Pass 1 }
  | INT       	  	{ Pass 1 }
  | FLT       	  	{ Pass 1 }
  | SYM       	  	{ Node sym 1 }
  | SYM '.' SYM	  	{ Node sym 1 3 }
  | VAR			{ Node var 1 }
  | ex OR ex  	  	{ Node or 1 3 }
  | ex AND ex  	  	{ Node and 1 3 }
  | ex EQ ex  	  	{ Node = 1 3 }
  | ex '<' ex  	  	{ Node < 1 3 }
  | ex '>' ex  	  	{ Node > 1 3 }
  | ex LE ex   	  	{ Node <= 1 3 }
  | ex GE ex   	  	{ Node >= 1 3 }
  | ex NE ex   	  	{ Node <> 1 3 }
  | ex '+' ex  	  	{ Node + 1 3}
  | ex '-' ex  	  	{ Node - 1 3 }
  | ex '*' ex  	  	{ Node * 1 3 }
  | ex '/' ex  	  	{ Node div 1 3 }
  | '!' ex   	  	{ Node 0= 2 }
  | '-' ex %prec NEG 	{ Node neg 2 }
  | '(' ex ')' 	  	{ Pass 2 }
  | '?'			{ Node bind }
  | SYM '(' ')'		{ Node fun 1 }
  | SYM '(' al ')'	{ Node fun 1 3 }
  ;
al: ae			{ Pass 1 }
  | ae ',' al		{ Join 1 3 }
  ;
ae: ex			{ Node arg 1 }
  ;
 
%%
