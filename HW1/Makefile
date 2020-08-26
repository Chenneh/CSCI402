warmup1: sort.o my402list.o
	gcc -o warmup1 -g sort.o my402list.o

sort.o: sort.c my402list.h
	gcc -g -c -Wall sort.c 

my402list.o: my402list.c my402list.h
	gcc -g -c -Wall my402list.c

clean:
	rm -f *.o warmup1