default:
	bison -vd maketop.ypp
	flex maketop.l
	g++ lex.yy.c maketop.tab.cpp -o maketop -ggdb -lfl
