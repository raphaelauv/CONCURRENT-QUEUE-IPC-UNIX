/* Par Juliusz Chroboczek, 2017. */

#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <complex.h>
#include <unistd.h>
#include <pthread.h>
#include "../conduct.h"

#define QSIZE 1000
#define COUNT 10


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


short int calcul(struct julia_request * req){
	short int result=0;
	for(int i=0;i<req->count;i++){
		result+=req->x*req->y;
	}
	return result;
}

 struct timespec t0;

static void * result_thread(void *arg){
	struct twocons cons = *(struct twocons*)arg;


	struct timespec t1;
	//printf("START RESULT\n");

    while(1) {
        struct julia_reply rep;
        int rc;

        rc = conduct_read(cons.two, &rep, sizeof(rep));
        if(rc <= 0) {
        	printf("ERROR RESULT\n");
        	conduct_write_eof(cons.two);
            return NULL;
        }

        if(rep.number > valueTotest -1){

    		clock_gettime(CLOCK_MONOTONIC, &t1);

    		printf("Repaint done in %.6lfs\n",
           ((double)t1.tv_sec - t0.tv_sec) +
           ((double)t1.tv_nsec - t0.tv_nsec) / 1.0E9);
    		break;
        }
        //printf("RESULT : %d\n",rep.number);

    }
    //printf("result : END\n");
    return NULL;
}

static void * order_thread(void *arg){
	struct twocons cons = *(struct twocons*)arg;
	int i=0;
	//conduct_show(cons.one);

	//printf("START ORDER\n");

    while(1) {
        struct julia_request req;
        req.number=i;
        i++;
        req.count=i*10;
        req.x=i%5;
        req.y=i%7;

        int rc;
		rc = conduct_write(cons.one, &req, sizeof(req));
		if (rc < 0) {
			printf("ERROR ORDER\n");
			conduct_write_eof(cons.one);
			return NULL;
		}

		if(i>valueTotest +nbASK ){

			
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
    struct twocons cons = *(struct twocons*)arg;

    //printf("START WORKER\n");

    while(1) {
        struct julia_request req;
        struct julia_reply rep;
        int rc;

        rc = conduct_read(cons.one, &req, sizeof(req));
        if(rc <= 0) {
        	printf("ERROR WORKER READ\n");
            conduct_write_eof(cons.two);
            return NULL;
        }


        rep.x=req.x;
        rep.y=req.y;
        rep.number=req.number;
        rep.count=req.count;
        rep.result=1;

        rc = conduct_write(cons.two, &rep, sizeof(rep));
        if(rc < 0) {
        	printf("ERROR WORKER WRITE\n");
            conduct_write_eof(cons.two);
            return NULL;
        }

        if(rep.number>valueTotest ){
			break;
		}

        //printf("WORKER : %d\n",req.number);
    }

    //printf("WORKER : END\n");
    return NULL;
}

int main(int argc, char **argv)
{
    
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

    if(numthreads <= 0)
        numthreads = sysconf(_SC_NPROCESSORS_ONLN);
    if(numthreads <= 0) {
        perror("sysconf(_SC_NPROCESSORS_ONLN)");
        exit(1);
    }
    printf("Running %d threads.\n", nbASK *3 *numthreads);

    numthreads*=nbASK * 3;

    nbASK = numthreads * nbASK ;

    pthread_t array[numthreads];

	
    for(int i = 0; i < numthreads; ) {


		rc += pthread_create(&( array[i]), NULL, order_thread, &cons);

		rc += pthread_create(&( array[i+1]), NULL, result_thread , &cons);

		rc += pthread_create(&( array[i+2]), NULL, worker_thread, &cons);

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

    return 0;
}

