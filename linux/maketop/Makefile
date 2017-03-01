default:
	bison -tvd maketop.ypp
	flex --header-file=lex.yy.h maketop.l
	g++ lex.yy.c maketop.tab.cpp Topology.cpp -o maketop -ggdb -lfl -Wall -std=c++11

clean:
	rm -f maketop
	rm -f maketop.output
	rm -f maketop.tab.cpp
	rm -f maketop.tab.hpp
	rm -f lex.yy.c
