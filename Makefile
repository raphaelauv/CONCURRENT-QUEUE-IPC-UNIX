CC=gcc

CFLAGS=`pkg-config gtk+-3.0 --cflags`

LDFLAGS=`pkg-config gtk+-3.0 --libs`

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
