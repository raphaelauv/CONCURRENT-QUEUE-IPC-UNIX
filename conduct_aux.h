#ifndef CONDUCT_AUX_H_
#define CONDUCT_AUX_H_


#define FILE_mode 0666

#define FLAG_CLEAN_DESTROY 1
#define FLAG_CLEAN_CLOSE 2

#define INTERNAL_FLAG_WRITE 1
#define INTERNAL_FLAG_READ 2
#define FLAG_WRITE_EOF 4
#define FLAG_WRITE_NORMAL 8
#define FLAG_O_NONBLOCK 16

#define MAXIMUM_SIZE_NAME_CONDUCT 100
#define LIMIT_SHOW "————————————————————————"

#include "conduct.h"


struct conduct{
	int size_mmap;
	char * fileName;
	void * mmap;
};

struct dataCirularBuffer{
	char passByMiddle;
	size_t count;
	size_t sizeAvailable;
	size_t firstMaxFor;
	size_t secondMaxFor;
	size_t sizeToManipulate;
	ssize_t sizeReallyManipulate;

	//for iter at end
	char * currentBuf;
	size_t currentCount;

};

struct content {
	pthread_mutex_t mutex;
	pthread_cond_t conditionRead;
	pthread_cond_t conditionWrite;

	int size_mmap;
	size_t sizeMax;
	size_t sizeAtom;
	size_t start;
	size_t end;

	char isEOF;
	char isEmpty;
	char *buffCircular;
};


int conduct_show(struct conduct *c);
extern inline void clean_Content(struct content * cont);
extern inline int copyFileName(const char * fileName ,struct conduct * cond);
extern inline void eval_size_to_manipulate(struct content * ct,struct dataCirularBuffer * data,int flag);
extern inline void eval_limit_loops(struct content * ct,struct dataCirularBuffer * data,int flag);
extern inline void eval_position_and_size_of_data(struct content * ct,struct dataCirularBuffer * data,int flag);
extern inline int init_dataCirularBuffer(struct dataCirularBuffer * data,struct conduct *c,const struct iovec *iov, int iovcnt,unsigned char flag);
extern inline void apply_loops(struct dataCirularBuffer * data,struct content *ct,const struct iovec *iov,unsigned char flag);

#endif /* CONDUCT_AUX_H_*/
