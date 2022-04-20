CC=g++
CFLAGS=-Wall -Wextra -O3
LIBS=-isystem benchmark/include -Lbenchmark/build/src -lbenchmark -lpthread
LOCATION=grenoble
MACHINE=yeti

upload:
	ninja -C build
	scp build/test1 build/test2 $(LOCATION).grid:psar/

fichiertest:
	# create 50M file
	dd if=/dev/urandom of=$@ status=progress bs=1024 count=50000

exec_test1:
	sudo-g5k ./build/test1

exec_test2:
	sudo-g5k ./build/test2

exec_all: exec_test1 exec_test2

plotres:
	python plot.py
