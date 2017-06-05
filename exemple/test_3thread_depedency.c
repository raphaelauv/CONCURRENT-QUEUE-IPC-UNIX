	/* Par Juliusz Chroboczek, 2017. */

#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <complex.h>
#include <unistd.h>
#include <pthread.h>

#include <asm-generic/socket.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>

#include "../conduct.h"


#define COUNT 10
#define QSIZE 100
#define MODE_COND 1
#define MODE_PIPE 0
#define MODE_SOCKET 0


//int valueTotest =10;
int valueTotest =100000;
int nbThreadsMultiplier = 10;
int nbASK = 0;


/* La structure qui contient une requête du programme principal. */

struct julia_request {
	int number;
    short int x, y;
    short int count;
};

/* La réponse d'un travailleur. */

struct julia_reply {
	int number;
    short int x, y;
    short int count;
    short int result;
};

struct twocons {
    struct conduct *one, *two;
};

struct twopipe {
     int *one, *two;
};

struct twosock {
	int *one, *two;
};


short int calcul(struct julia_request * req){
	short int result=0;
	/*
	for(int i=0;i<req->count;i++){
		result+=req->x*req->y;
	}
	*/
	return result;
}

 struct timespec t0;

static void * result_thread(void *arg){
	struct twocons cons;
	struct twopipe pipes;
	struct twosock socks;

	if (MODE_COND) {
		cons = *(struct twocons*)arg;
	} else if (MODE_PIPE) {
		pipes = *(struct twopipe*)arg;
	} else if (MODE_SOCKET) {
		socks = *(struct twosock*)arg;
	}

	
	//printf("START RESULT\n");

	int nbDone=0;
	int retry;
    while(1) {
        struct julia_reply rep;
        int rc;

		if (MODE_COND) {

			retry=1;
			while (retry) {
				rc = conduct_read_nb(cons.two, &rep, sizeof(rep));
				if (rc == -1 && errno == EWOULDBLOCK) {
					sched_yield();
				} else {
					retry = 0;
				}
			}

		} else if (MODE_PIPE) {
			rc = read(pipes.two[0], &rep, sizeof(rep));
		} else if (MODE_SOCKET) {
			rc = read(socks.two[0], &rep, sizeof(rep));
		}


        if(rc <= 0) {
        	printf("ERROR RESULT\n");
        	conduct_write_eof(cons.two);
            return NULL;
        }

        if(rc!= sizeof(rep)){
        	printf("ERROR RESULT\n");
        	return NULL;
        }

        nbDone++;
        if(nbDone > valueTotest -1){
    		break;
        }
        //printf("RESULT : %d\n",rep.number);

    }
    		struct timespec t1;

			clock_gettime(CLOCK_MONOTONIC, &t1);

    		printf("Repaint done in %.6lfs\n",
           ((double)t1.tv_sec - t0.tv_sec) +
           ((double)t1.tv_nsec - t0.tv_nsec) / 1.0E9);


    //printf("result : END\n");
    return NULL;
}

static void * order_thread(void *arg){

	struct twocons cons;
	struct twopipe pipes;
	struct twosock socks;

	if (MODE_COND) {
		cons = *(struct twocons*)arg;
	} else if (MODE_PIPE) {
		pipes = *(struct twopipe*)arg;
	} else if (MODE_SOCKET) {
		socks = *(struct twosock*)arg;
	}


	int i=0;

	int retry;
    while(1) {
        struct julia_request req;
        req.number=i;
        i++;
        req.count=i;
        req.x=i;
        req.y=i;

        int rc;

		if (MODE_COND) {


			retry=1;
			while (retry) {
				rc = conduct_write_nb(cons.one, &req, sizeof(req));
				if (rc == -1 && errno == EWOULDBLOCK) {
					sched_yield();
				} else {
					retry = 0;
				}
			}


		} else if (MODE_PIPE) {
			rc =write(pipes.one[1], &req, sizeof(req));
		} else if (MODE_SOCKET) {
			rc = write(socks.one[1], &req, sizeof(req));
		}

		if (rc < 0) {
			printf("ERROR ORDER\n");
			conduct_write_eof(cons.one);
			return NULL;
		}

		if (rc != sizeof(req)) {
			printf("ERROR ORDER\n");
			return NULL;
		}

		if(i>valueTotest ){

			
			break;
		}
		//printf("ORDER : %d\n",req.number);
		//conduct_show(cons.one);

    }
	//printf("order : END\n");
    return NULL;
}

static void * worker_thread(void *arg)
{
	struct twocons cons;
	struct twopipe pipes;
	struct twosock socks;

	if (MODE_COND) {
		cons = *(struct twocons*)arg;
	} else if (MODE_PIPE) {
		pipes = *(struct twopipe*)arg;
	} else if (MODE_SOCKET) {
		socks = *(struct twosock*)arg;
	}
	int retry;
	int nbDone=0;
    while(1) {
        struct julia_request req;
        struct julia_reply rep;
        int rc;


		if (MODE_COND) {

			retry=1;

			while (retry) {
				rc = conduct_read_nb(cons.one, &req, sizeof(req));
				if (rc == -1 && errno == EWOULDBLOCK) {
					sched_yield();
				} else {
					retry = 0;
				}
			}

		} else if (MODE_PIPE) {
			rc = read(pipes.one[0], &req, sizeof(req));
		} else if (MODE_SOCKET) {
			rc = read(socks.one[0], &req, sizeof(req));
		}

        if(rc <= 0) {
        	printf("ERROR WORKER READ\n");
            conduct_write_eof(cons.two);
            return NULL;
        }

        if (rc != sizeof(req)) {
			printf("ERROR WOKER\n");
			return NULL;
		}


        rep.x=req.x;
        rep.y=req.y;
        rep.number=req.number;
        rep.count=req.count;
        rep.result=1;



		if (MODE_COND) {

			int retry=1;
			while(retry){
				rc = conduct_write_nb(cons.two, &rep, sizeof(rep));
				if(rc==-1 && errno==EWOULDBLOCK){
					sched_yield();
				}else if(rc==-1){
					printf("ERRNO PAS A EWOULD\n");
				}else{
					retry=0;
				}
			}


		} else if (MODE_PIPE) {
			rc =write(pipes.two[1], &rep, sizeof(rep));
		} else if (MODE_SOCKET) {
			rc = write(socks.two[1], &rep, sizeof(rep));
		}


        if(rc < 0) {
        	printf("ERROR WORKER WRITE\n");
            conduct_write_eof(cons.two);
            return NULL;
        }

        if (rc != sizeof(rep)) {
			printf("ERROR WOKER\n");
			return NULL;
		}

		nbDone++;
        if(nbDone>valueTotest -1 ){
			break;
		}

        //printf("WORKER : %d\n",req.number);
    }

    //printf("WORKER : END\n");
    return NULL;
}

int main(int argc, char **argv)
{

	/******************PIPE************************/
	int descriP1[2];
	int descriP2[2];
	if (pipe(descriP1) || pipe(descriP2)) {
		perror("pipe eror");
		return 1;
	}

	struct twopipe pipes;
	pipes.one=descriP1;
	pipes.two=descriP2;


	/******************SOCKET************************/




	char * server_filename = "/tmp/socket-server1";
	char * client_filename = "/tmp/socket-client1";

	unlink(server_filename);
	unlink(client_filename);

	struct sockaddr_un server_addr;
	struct sockaddr_un client_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sun_family = AF_UNIX;
	strncpy(server_addr.sun_path, server_filename, 104); // XXX: should be limited to about 104 characters, system dependent

	memset(&client_addr, 0, sizeof(client_addr));
	client_addr.sun_family = AF_UNIX;
	strncpy(client_addr.sun_path, client_filename, 104);

	// get socket
	int sockclient1 = socket(AF_UNIX, SOCK_DGRAM, 0);
	if(sockclient1<0){
		perror("bind failed");
	}

	int sockserv1 = socket(AF_UNIX, SOCK_DGRAM, 0);
	if(sockserv1<0){
		perror("soket failed");
	}

	// bind
	if(bind(sockclient1, (struct sockaddr *) &client_addr, sizeof(client_addr))){
		 perror("bind failed");
	}
	if(bind(sockserv1, (struct sockaddr *) &server_addr, sizeof(server_addr))){
		perror("bind failed");
	}

	// connect client to server_filename
	connect(sockclient1, (struct sockaddr *) &server_addr, sizeof(server_addr));
	//connect(sockserv1, (struct sockaddr *) &client_addr, sizeof(client_addr));


	server_filename = "/tmp/socket-server2";
	client_filename = "/tmp/socket-client2";

	unlink(server_filename);
	unlink(client_filename);


	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sun_family = AF_UNIX;
	strncpy(server_addr.sun_path, server_filename, 104); // XXX: should be limited to about 104 characters, system dependent

	memset(&client_addr, 0, sizeof(client_addr));
	client_addr.sun_family = AF_UNIX;
	strncpy(client_addr.sun_path, client_filename, 104);

	// get socket
	int sockclient2 = socket(AF_UNIX, SOCK_DGRAM, 0);
	if(sockclient2<0){
		perror("bind failed");
	}
	int sockserv2 = socket(AF_UNIX, SOCK_DGRAM, 0);
	if(sockserv2<0){
		perror("soket failed");
	}

	// bind client to client_filename
	bind(sockclient2, (struct sockaddr *) &client_addr, sizeof(client_addr));
	bind(sockserv2, (struct sockaddr *) &server_addr, sizeof(server_addr));

	// connect client to server_filename
	connect(sockclient2, (struct sockaddr *) &server_addr, sizeof(server_addr));
	//connect(sockserv2, (struct sockaddr *) &client_addr, sizeof(client_addr));

	struct twosock socks;
	int descriS1[2];
	int descriS2[2];

	descriS1[0]=sockserv1;
	descriS1[1]=sockclient1;
	descriS2[0]=sockserv2;
	descriS2[1]=sockclient2;


	socks.one=descriS1;
	socks.two=descriS2;
    
	/******************CONDUCT************************/
    struct twocons cons;
    int numthreads = 0;
    int rc;

    nbASK=nbThreadsMultiplier;
    if(argc>1){

    	nbASK=atoi(argv[1]);

    	printf("ARG : %d\n",nbASK);
    }


    cons.one = conduct_create(NULL, sizeof(struct julia_request),
                              QSIZE * sizeof(struct julia_request));
    if(cons.one == NULL) {
        perror("conduct_create");
        exit(1);
    }

    cons.two = conduct_create(NULL, sizeof(struct julia_reply),
                              QSIZE * sizeof(struct julia_reply));

    if(cons.two == NULL) {
        perror("conduct_create");
        exit(1);
    }


    /******************SELECTION OF MODE************************/
    void * structToUse;
    if(MODE_COND){
    	structToUse=&cons;
    }
    else if(MODE_PIPE){
    	structToUse=&pipes;
    }
    else if(MODE_SOCKET){
    	structToUse=&socks;
    }


    /******************THREADS************************/
    if(numthreads <= 0)
        numthreads = sysconf(_SC_NPROCESSORS_ONLN);
    if(numthreads <= 0) {
        perror("sysconf(_SC_NPROCESSORS_ONLN)");
        exit(1);
    }
    

    numthreads=3*nbASK;

    pthread_t array[numthreads];

    printf("Running %d threads.\n", numthreads);
	
    for(int i = 0; i < numthreads; ) {


		rc += pthread_create(&( array[i]), NULL, order_thread, structToUse);

		rc += pthread_create(&( array[i+1]), NULL, result_thread , structToUse);

		rc += pthread_create(&( array[i+2]), NULL, worker_thread, structToUse);

        if(rc != 0) {
            errno = rc;
            perror("pthread_create");
            exit(1);
        }
        i+=3;

    }

    clock_gettime(CLOCK_MONOTONIC, &t0);

    for(int i = 0; i < numthreads; i++) {
		pthread_join(array[i], NULL);
    }
    /*
	struct timespec t1;
    clock_gettime(CLOCK_MONOTONIC, &t1);

    		printf("Repaint done in %.6lfs\n",
           ((double)t1.tv_sec - t0.tv_sec) +
           ((double)t1.tv_nsec - t0.tv_nsec) / 1.0E9);

     */

    return 0;
}

