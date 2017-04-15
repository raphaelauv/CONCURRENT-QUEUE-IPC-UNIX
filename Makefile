CC=gcc

CFLAGS=-g -O3 -ffast-math -Wall -pthread `pkg-config gtk+-3.0 --cflags`

LDFLAGS=`pkg-config gtk+-3.0 --libs` -lm -lrt 

SRC = conduct.c

SRC_CONCURRENT = concurrentconduct.c

SRC_ASK = $(SRC)

OBJ = $(SRC_ASK:.c=.o)

EXEC = julia test little_test

EXEC_CONC = $(eval SRC_ASK=$(SRC_CONCURRENT)) $(EXEC)

all: $(EXEC)

conc: $(EXEC_CONC)


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
