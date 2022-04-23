CC=g++
CFLAGS=-Wall -Wextra -O3
LIBS=-isystem benchmark/include -Lbenchmark/build/src -lbenchmark -lpthread
LOCATION=grenoble
MACHINE=yeti

fichiertest:
	# create 50M file
	dd if=/dev/urandom of=$@ status=progress bs=1024 count=50000

exec:
	sudo-g5k ./build/test_distant_reads_distant_buffer_forced
	sudo-g5k ./build/test_distant_reads_distant_buffer
	sudo-g5k ./build/test_distant_reads_local_buffer

plotres:
	python plot.py
