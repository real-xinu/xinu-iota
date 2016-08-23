default:
	bison -vd maketop.ypp
	flex maketop.l
	g++ lex.yy.c maketop.tab.cpp -o maketop -ggdb -lfl

clean:
	rm -f maketop
	rm -f maketop.output
	rm -f maketop.tab.cpp
	rm -f maketop.tab.hpp
	rm -f lex.yy.c
