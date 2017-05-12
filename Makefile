CC=gcc 
VERSION=-std=gnu11
CFLAGS= $(VERSION) -g -O3 -ffast-math -Wall -pthread `pkg-config gtk+-3.0 --cflags`

LDFLAGS=`pkg-config gtk+-3.0 --libs` -lm -lrt 

SRC_ASK =  conduct.c #concurrentconduct.c

SRC = $(SRC_ASK)
OBJ = $(SRC_ASK:.c=.o)	


EXEC =  julia test_3thread_depedency test_simple test_vectored_IO

all:$(EXEC)

julia: $(OBJ)  exemple/julia.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

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
