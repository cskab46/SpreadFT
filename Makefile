CC=gcc
LD=gcc
CFLAGS=-g -Wall -std=c99
CPPFLAGS=-I. -I/home/eschemb1/distributed/project3/include
SP_LIBRARY_DIR=/home/eschemb1/distributed/project3

all: mcast

mcast:  $(SP_LIBRARY_DIR)/libspread-core.a mcast.o
	$(LD) -o $@ mcast.o $(SP_LIBRARY_DIR)/libspread-core.a -ldl -lm -lrt -lnsl $(SP_LIBRARY_DIR)/libspread-util.a

clean:
	rm -f *.o mcast

