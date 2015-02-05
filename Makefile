#Start of the Makefile


obj-m += virusScanner.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

#all: virusScanner.o 
#	gcc -std=c99 -o virusScanner virusScanner.o -I.
	
#virusScanner.o: virusScanner.c
#	gcc -std=c99 -Wall -g -c virusScanner.c -I.
	
#clean:
#	rm *.o

##End of the Makefile

