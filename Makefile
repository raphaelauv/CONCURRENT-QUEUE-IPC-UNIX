CC=gcc 
VERSION=-std=gnu11
CFLAGS= $(VERSION) -g -O3 -ffast-math -Wall -pthread 

CFLAGS_GTK = $(CFLAGS) `pkg-config gtk+-3.0 --cflags`

LDFLAGS=-lm -lrt

LDFLAGS_GTK = `pkg-config gtk+-3.0 --libs` $(LDFLAGS)  

SRC_CONC =  concurrentconduct.c sortedLinkedList.c

OBJ_CONC = $(SRC_CONC:.c=.o)

SRC_N = conduct.c

OBJ_N = $(SRC_N:.c=.o)

EXEC =  julia test_3thread_depedency test_simple test_vectored_IO

EXEC_C =  julia_C test_3thread_depedency_C test_simple_C test_vectored_IO_C

all:$(EXEC)

conc: $(EXEC_C)

exemple/julia.o:
	$(CC) $(CFLAGS_GTK) -c exemple/julia.c -o $@

#----------------------------N----------------------------#

julia: $(OBJ_N)  exemple/julia.o
	$(CC) $(CFLAGS_GTK) -o $@ $^ $(LDFLAGS_GTK)

test_3thread_depedency: $(OBJ_N)  exemple/test_3thread_depedency.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test_vectored_IO: $(OBJ_N)  exemple/test_vectored_IO.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test_simple	: $(OBJ_N)  exemple/test_simple.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)


#---------------------------CONC---------------------------#

julia_C: $(OBJ_CONC)  exemple/julia.o
	$(CC) $(CFLAGS_GTK) -o $@ $^ $(LDFLAGS_GTK)

test_3thread_depedency_C: $(OBJ_CONC)  exemple/test_3thread_depedency.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test_vectored_IO_C: $(OBJ_CONC)  exemple/test_vectored_IO.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test_simple_C	: $(OBJ_CONC)  exemple/test_simple.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

#---------------------------------------------------------#


.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	find . -name '*.o' -delete

mrproper: clean
	rm -rf $(EXEC)
	rm -rf $(EXEC_C)
