/* Par Juliusz Chroboczek, 2017. */

#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <complex.h>
#include <unistd.h>
#include <pthread.h>
#include "conduct.h"

#define ITERATIONS 1000
#define QSIZE 100
#define COUNT 100

/* Voir la fonction toc ci-dessous. */

int dx, dy;
complex double julia_c;

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


static void * result_thread(void *arg){
	struct twocons cons = *(struct twocons*)arg;
    while(1) {
        struct julia_reply rep;
        int rc;

        rc = conduct_read(cons.two, &rep, sizeof(rep));
        if(rc <= 0) {
        	conduct_write_eof(cons.two);
            return NULL;
        }

        printf("RESULT : %d\n",rep.number);

    }
}

static void * order_thread(void *arg){
	struct twocons cons = *(struct twocons*)arg;
	int i=0;
	//conduct_show(cons.one);
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
			conduct_write_eof(cons.one);
			return NULL;
		}
		printf("ORDER : %d\n",req.number);
		//conduct_show(cons.one);

    }
}

static void * julia_thread(void *arg)
{
    struct twocons cons = *(struct twocons*)arg;
    while(1) {
        struct julia_request req;
        struct julia_reply rep;
        int rc;

        rc = conduct_read(cons.one, &req, sizeof(req));
        if(rc <= 0) {
            conduct_write_eof(cons.two);
            return NULL;
        }

        rep.x=req.x;
        rep.y=req.y;
        rep.number=req.number;
        rep.count=req.count;
        rep.result=calcul(&req);

        rc = conduct_write(cons.two, &rep, sizeof(rep));
        if(rc < 0) {
            conduct_write_eof(cons.two);
            return NULL;
        }
    }
}


int main(int argc, char **argv)
{
    
    struct twocons cons;
    int numthreads = 0;
    int rc;

    julia_c = 1.0 - (1.0 + sqrt(5.0)) / 2.0;



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
    printf("Running %d worker threads.\n", numthreads);

    pthread_t array[numthreads+2];

    for(int i = 0; i < numthreads+2; i++) {

    	if(i==0){
    		rc = pthread_create(&( array[i]), NULL, order_thread, &cons);
    	}else if(i==1){
    		rc = pthread_create(&( array[i]), NULL, result_thread , &cons);
    	}else{
    		rc = pthread_create(&( array[i]), NULL, julia_thread, &cons);
    	}

        if(rc != 0) {
            errno = rc;
            perror("pthread_create");
            exit(1);
        }

    }
    for(int i = 0; i < numthreads+2; i++) {
		pthread_join(array[i], NULL);
    }

    return 0;
}
