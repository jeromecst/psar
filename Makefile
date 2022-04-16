CC=g++
CFLAGS=-Wall -Wextra -O3
LIBS=-isystem benchmark/include -Lbenchmark/build/src -lbenchmark -lpthread
LOCATION=lyon

all: test test2

test: test.o fichiertest
	$(CC) $(CFLAGS) $< -o $@ $(LIBS)

test2: test2.o fichiertest
	$(CC) $(CFLAGS) $< -o $@ $(LIBS)

upload: test.cpp test2.cpp Makefile test.h
	scp $? $(LOCATION).grid:psar/

fichiertest:
	# create 50M file
	dd if=/dev/urandom of=$@ status=progress bs=1024 count=50000

libs:
	git clone https://github.com/google/benchmark
	cmake -E make_directory "benchmark/build"
	# Generate build system files with cmake, and download any dependencies.
	cmake -E chdir "benchmark/build" cmake -DBENCHMARK_DOWNLOAD_DEPENDENCIES=on -DCMAKE_BUILD_TYPE=Release ../
	# or, starting with CMake 3.13, use a simpler form:
	# cmake -DCMAKE_BUILD_TYPE=Release -S . -B "build"
	# Build the library.
	cmake --build "benchmark/build" --config Release
	sudo-g5k cmake --build "benchmark/build" --config Release --target install

exec_test:
	sudo-g5k ./test --benchmark_out=test_result.js --benchmark_out_format=json

exec_test2:
	sudo-g5k ./test2 --benchmark_out=test2_result.js --benchmark_out_format=json

exec_all: exec_test exec_test2


clean:
	rm -f *.o
