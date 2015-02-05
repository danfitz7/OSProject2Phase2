#Start of the Makefile
all: testSmite.o
	gcc -std=c99 -o testSmite testSmite.o -I.

runCommand.o: testSmite.c
	gcc -std=c99 -Wall -g -c testSmite.c -I.

clean:
	rm *.o

##End of the Makefile

