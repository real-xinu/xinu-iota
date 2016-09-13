%{
#include "maketop.tab.hpp"
#include <stdio.h>
#define YY_DECL extern "C" int yylex()

#ifdef DEBUG
#  define debug(x) x
#else
#  define debug(x)
#endif

%}
%%
[ \t\n]         ;
"+"		{ debug(printf("Found +\n")); return(ADD); }
"-"		{ debug(printf("Found -\n")); return(DELETE); }
"<-"		{ debug(printf("Found left arrow %s\n", yytext)); return(LEFT); }
"->"		{ debug(printf("Found right arrow %s\n", yytext)); return(RIGHT);}
"<->"		{ debug(printf("Found bi arrow %s\n", yytext)); return(BI);}
"="		{ debug(printf("Found equals %s\n", yytext)); return(EQ);}
"("		{ debug(printf("Found left paren %s\n", yytext)); return(LEFTPAREN);}
")"		{ debug(printf("Found right paren %s\n", yytext)); return(RIGHTPAREN);}
"maketop"	{ debug(printf("Found maketop\n")); return(MAKETOP);}
"ring"		{ debug(printf("Found ring")); return(RING);}
"linear"	{ debug(printf("Found linear")); return(LINEAR);}
"complete"	{ debug(printf("Found complete")); return(COMPLETE);}
"star"		{ debug(printf("Found star")); return(STAR);}

[a-zA-Z][a-zA-Z0-9_]*	{ debug(printf("Node %s\n", yytext));
			  yylval.node_name_value = strdup(yytext);
			  return(NODE);}

[0-9]+\.[0-9]+  { debug(printf("Found a floating-point number:%s\n", yytext)); }
[0-9]+          { debug(printf("Found an integer:%s\n", yytext)); yylval.ival = atoi(yytext); return(INT); }
[a-zA-Z0-9]+    { debug(printf("Found a string: %s\n", yytext)); }

%%
