warmup2: emulation.o my402list.o
	gcc -o test -g test.o -Wall -pthread

emulation.o: emulation.c emulation.h my402list.h
	gcc -g -c -Wall emulation.c

my402list.o: my402list.c my402list.h
	gcc -g -c -Wall my402list.c

clean:
	rm -f *.o test