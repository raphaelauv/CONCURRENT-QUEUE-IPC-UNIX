CC=gcc 
VERSION=-std=gnu11
CFLAGS= $(VERSION) -g -O3 -ffast-math -Wall -pthread 

CFLAGS_GTK = $(CFLAGS) `pkg-config gtk+-3.0 --cflags`

LDFLAGS=-lm -lrt

LDFLAGS_GTK = `pkg-config gtk+-3.0 --libs` $(LDFLAGS)  

SRC_ASK =  concurrentconduct.c #conduct.c

OBJ = $(SRC_ASK:.c=.o)	

EXEC =  julia test_3thread_depedency test_simple test_vectored_IO

all:$(EXEC)

exemple/julia.o:
	$(CC) $(CFLAGS_GTK) -c exemple/julia.c -o $@

julia: $(OBJ)  exemple/julia.o
	$(CC) $(CFLAGS_GTK) -o $@ $^ $(LDFLAGS_GTK)

test_3thread_depedency: $(OBJ)  exemple/test_3thread_depedency.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test_vectored_IO: $(OBJ)  exemple/test_vectored_IO.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test_simple	: $(OBJ)  exemple/test_simple.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)



.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	find . -name '*.o' -delete

mrproper: clean
	rm -rf $(EXEC)
