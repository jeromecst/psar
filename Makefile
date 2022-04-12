all:
	g++ -Wall -Wextra -O3 test.cpp -isystem benchmark/include -Lbenchmark/build/src -lbenchmark -lpthread
