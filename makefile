rrs: formula.o rrs.cc rrs.hh
	g++ -O3 -Wpedantic -static-libstdc++ -std=c++11 -g rrs.cc formula.o -o rrs

formula.o: formula.cc formula.hh
	g++ -Wall -O3 -std=c++11 -c -g formula.cc

clean:
	rm --force formula.o rrs
