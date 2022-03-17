all:
	g++ -Wall -Wextra -O3 -lpthread -isystem benchmark/include -Lbenchmark/build/src -lbenchmark test.cpp 
