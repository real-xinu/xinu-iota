%{
#include "maketop.tab.hpp"
#include <stdio.h>
#define YY_DECL extern "C" int yylex()

%}
%%
[ \t\n]         ;
"<-"		{ printf("Found left arrow %s\n", yytext); return(LEFT); }
"->"		{ printf("Found right arrow %s\n", yytext); return(RIGHT);}
"<->"		{ printf("Found bi arrow %s\n", yytext); return(BI);}
"="		{ printf("Found equals %s\n", yytext); return(EQ);}
"("		{ printf("Found left paren %s\n", yytext); return(LEFTPAREN);}
")"		{ printf("Found right paren %s\n", yytext); return(RIGHTPAREN);}
"creategraph"	{ printf("Found creategraph\n"); return(CREATEGRAPH);}
"ring"		{ printf("Found ring"); return(RING);}
"linear"	{ printf("Found linear"); return(LINEAR);}
"complete"	{ printf("Found complete"); return(COMPLETE);}

[a-zA-Z][a-zA-Z0-9]*	{ printf("Node %s\n", yytext);
			  yylval.node_name_value = strdup(yytext);
			  return(NODE);}

[0-9]+\.[0-9]+  { printf("Found a floating-point number:%s\n", yytext); }
[0-9]+          { printf("Found an integer:%s\n", yytext); yylval.ival = atoi(yytext); return(INT); }
[a-zA-Z0-9]+    { printf("Found a string: %s\n", yytext); }

%%
