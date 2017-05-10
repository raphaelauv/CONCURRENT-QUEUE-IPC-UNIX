CC=gcc 
VERSION=-std=gnu11
CFLAGS= $(VERSION) -g -O3 -ffast-math -Wall -pthread `pkg-config gtk+-3.0 --cflags`

LDFLAGS=`pkg-config gtk+-3.0 --libs` -lm -lrt 

SRC_ASK = conduct.c #concurrentconduct.c 

SRC = $(SRC_ASK)
OBJ = $(SRC_ASK:.c=.o)	


EXEC =  julia test little_test

all:$(EXEC)

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
