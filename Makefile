CC=gcc

CFLAGS=-g -O3 -ffast-math -Wall -pthread `pkg-config gtk+-3.0 --cflags`

LDFLAGS=`pkg-config gtk+-3.0 --libs` -lm -lrt 

SRC = concurrentconduct.c #conduct.c conduct_aux.c conduct_IO.c

OBJ = $(SRC:.c=.o)

EXEC = julia test little_test

all: $(EXEC)

julia: $(OBJ)  exemple/julia.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test: $(OBJ)  exemple/test.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

little_test	: $(OBJ)  exemple/little_test.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	find . -name '*.o' -delete

mrproper: clean
	rm -rf $(EXEC)
