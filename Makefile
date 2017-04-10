CC=gcc

CFLAGS=-g -O3 -ffast-math -Wall -pthread `pkg-config gtk+-3.0 --cflags`

LDFLAGS=`pkg-config gtk+-3.0 --libs` -lm -lrt 

SRC = conduct.c

OBJ = $(SRC:.c=.o)

EXEC = julia

all: $(EXEC)

julia: $(OBJ)  julia.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	find . -name '*.o' -delete

mrproper: clean
	rm -rf $(EXEC)
