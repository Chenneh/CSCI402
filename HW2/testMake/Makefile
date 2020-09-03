test: test.o
	gcc -o test -g test.o -Wall -pthread

test.o: test.c
	gcc -g -c -Wall test.c 

clean:
	rm -f *.o test