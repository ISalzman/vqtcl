%token SYM INT STR FLT VAR OPS _ALT_
%token OR AND NOT SELECT FROM AS WHERE UNION EXCEPT GROUP BY HAVING ORDER
%token AVG COUNT SUM MIN MAX EXISTS ANY ALL ASC DESC
%token _OPS_ LE GE NE

%left OR
%left AND
%left NOT
%left '=' '<' '>' LE GE NE
%left '+' '-'
%left '*' '/'
%left NEG       # negation--unary minus

%%

go: st				{ Pass 1 }
  ;
st: ss				{ Pass 1 }
  | ss UNION ss			{ Node uni 1 3 }
  | ss EXCEPT ss		{ Node exc 1 3 }
  ;
ss: SELECT cl fr wh gb hv ob	{ Node sel 2 3 4 5 6 7 }
  ;
cl: '*'				{ Node star }
  | ci				{ Pass 1 }
  | ci ',' cl			{ Join 1 3 }
  ;
ci: ex				{ List 1 0 }
  | ex AS SYM		        { List 1 3 }
  ;
ol: oi			        { Pass 1 }
  | oi ',' ol		        { Join 1 3 }
  ;
oi: ex			        { Node sort 1 }
  | ex ASC		        { Node asc 1 }
  | ex DESC		        { Node desc 1 }
  ;
tl: ti			        { Pass 1 }
  | ti ',' tl		        { Join 1 3 }
  ;
ti: tn				{ List 1 0 }
  | tn SYM		        { List 1 2 }
  | tn AS SYM			{ List 1 3 }
  ;
tn: SYM				{ Pass 1 }
  | '(' ss ')'			{ Node sub 2 }
  ;
fr:			        { List }
  | FROM tl		        { Pass 2 }
  ;
wh:			        { List }
  | WHERE ex		        { Pass 2 }
  ;
gb:			        { List }
  | GROUP BY cl		        { Pass 3 }
  ;
hv:			        { List }
  | HAVING ex		        { Pass 2 }
  ;
ob:			        { List }
  | ORDER BY ol		        { Pass 3 }
  ; 
ex: INT			        { Node int 1 }
  | FLT			        { Node flt 1 }
  | STR			        { Node str 1 }
  | SYM			        { Node sym 1 }
  | SYM '.' SYM		        { Node dot 1 3 }
  | VAR				{ Node var 1 }
  | ex OR ex		        { Node or 1 3 }
  | ex AND ex		        { Node and 1 3 }
  | ex '=' ex		        { Node = 1 3 }
  | ex '<' ex		        { Node < 1 3 }
  | ex '>' ex		        { Node > 1 3 }
  | ex LE ex		        { Node <= 1 3 }
  | ex GE ex		        { Node >= 1 3 }
  | ex NE ex		        { Node <> 1 3 }
  | ex '+' ex	 	        { Node + 1 3 }
  | ex '-' ex  	  	        { Node - 1 3 }
  | ex '*' ex  	  	        { Node * 1 3 }
  | ex '/' ex  	  	        { Node div 1 3 }
  | NOT ex		        { Node 0= 2 }
  | '-' ex %prec NEG 	        { Node neg 2 }
  | '(' ex ')' 	  	        { Pass 2 }
  | EXISTS '(' ss ')'	        { Node exi 3 }
  | AVG '(' ex ')'	        { Node avg 3 }
  | SUM '(' ex ')'	        { Node sum 3 }
  | MIN '(' ex ')'	        { Node min 3 }
  | MAX '(' ex ')'	        { Node max 3 }
  | COUNT '(' ex ')'	        { Node cnt 3 }
  | COUNT '(' '*' ')'	        { Node num }
  | '(' ss ')'			{ Node sub 2 }
  ;

%%
