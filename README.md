# IPC -> INTER-PROCESSUS COMMUNICATIONS


## ATOMIC CIRCULAR-BUFFER Message queue IMPLEMENTATIONS (UNIX)


- Custom Atomic Guaranteed limit
- Thread Safe ( Multiprocessing or Multithreading)
- Blocking 		read,write,write_eof
- Non Blocking 	read,write,write_eof
- Vectored 		read,write

- Compliant operations (try to read or write as far as ask by user while respecting atomic limit and blocking mode)
- Message queue can be anonymous or maped in a file
- Code Correctness (All systemCall and Mutex Operations return values are tested)


## IMPLEMENTATIONS						

### 1 ) conduct.c is not concurrent and not lock-free

	- The define MODE_MEMCPY at TRUE use the memcpy function from standard C library  else it don't


### 2) concurrentconduct.c is concurrent and not lock-free  (Writer and Reader can use at the same time the Circular-Buffer)

	- It use ATOMIC OPERATIONS
	- There is 2 implementation possible :

		MPMC ->  multiple producer multiple consumer solutions
		SPSC ->  single producer single consumer solutions


## COMPILATION

	make 
	without argument , compile the test FILES with the not concurrent implementation of conducts

	make conc
	with argument "conc" , compile the test FILES with the concurrent implementation of conducts


### TEST FILES

#### - julia
	#define MODE_FILE 1		named file mode
	#define MODE_BLOCK 1	mode blocking


#### - test_3thread_depedency	launch 3 type of threads with 2 shared conduct

	threads ORDER write in Conduct 1 || threads WORKER read in Conduct 1 and after write in conduct 2 || threads RESULT read Conduct 2
	
	#define QSIZE 100  		the number of oder per threads
	#define MODE_COND 1		mode conduct is activate , others are not
	#define MODE_PIPE 0
	#define MODE_SOCKET 0


#### - test_vectored_IO

#### - test_simple



## POSSIBLE IMPROVEMENT

	- Robust Mutex
	- Signals corectness



strace -c -f ./julia
time ./julia




