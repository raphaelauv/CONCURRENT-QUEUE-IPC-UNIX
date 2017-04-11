#include "conduct.h"

#define FILE_mode 0666
#define mode_ANONYMOUS 1
#define mode_NAMED 2
#define FLAG_CLEAN_DESTROY 1
#define FLAG_CLEAN_CLOSE 2

struct conduct{
	char modeMMAP;
	size_t c; //SIZE MAX PIPE
	size_t a; //ATOMIC MIN SIZE
	int fd;
	int sizeMMAP;
	void * mmap;
};

struct content {
	int var;
	pthread_mutex_t mutex;
	pthread_cond_t condition;

	int sizeMMAP;
	size_t sizeMax;
	size_t sizeAtom;

	char isEOF;
	char isEmpty;
	size_t start;
	size_t end;

	char *buffCircular;
};



void clean_Content(struct content * cont) {
	if (cont != NULL) {
		pthread_mutex_destroy(&cont->mutex);
		pthread_cond_destroy(&cont->condition);
	}
}

void clean_Conduct(struct conduct * cond,int flag) {
	if (cond != NULL) {

		if (cond->mmap != MAP_FAILED) {

			if(msync(cond->mmap,cond->mmap,MS_SYNC)){
				printf("ERROR msync()\n");
			}

			if(flag==FLAG_CLEAN_DESTROY){
				if (munmap(cond->mmap, cond->c)) {
					printf("ERROR munmap()\n");
				}
			}

		}


		if (cond->fd != -1) {
			close(cond->fd);
		}

		free(cond);
		cond = NULL;
	}
}

struct conduct *conduct_create(const char *name, size_t a, size_t c) {

	struct content * cont=NULL;
	struct conduct * cond=NULL;

	cond = (struct conduct *) malloc(sizeof(struct conduct));
	if (cond == NULL) {
		return NULL;
	}

	cond->c = c;
	cond->a = a;
	cond->fd = -1;

	cond->mmap=MAP_FAILED;
	cond->sizeMMAP=cond->c;
	cond->sizeMMAP+=sizeof(struct content);

	if (name != NULL) {

		cond->modeMMAP = mode_NAMED;
		cond->fd = open(name,O_CREAT | O_RDWR | O_TRUNC,FILE_mode);

		if (cond->fd < 0) {
			goto cleanup;
		}

		struct stat sb;
		off_t offset, pa_offset;

		if (fstat(cond->fd, &sb) == -1){
			goto cleanup;
		}


		offset = cond->sizeMMAP;
		//pa_offset = offset & ~(sysconf(_SC_PAGE_SIZE) - 1);

		if (offset >= sb.st_size) {
			if (ftruncate(cond->fd, cond->sizeMMAP)) {
				goto cleanup;
			}
			/*
			//TODO
			close(cond->fd);
			cond->fd = shm_open(name, O_CREAT | O_RDWR | O_TRUNC, FILE_mode);
			if (cond->fd < 0) {
				goto cleanup;
			}
			*/
		}

		cond->mmap = mmap(NULL, cond->sizeMMAP/*TODO*/ , PROT_WRITE | PROT_READ,MAP_SHARED, cond->fd, 0);

		if (cond->mmap == MAP_FAILED) {
			goto cleanup;
		}

	} else {

		cond->modeMMAP = mode_ANONYMOUS;
		cond->mmap = mmap( NULL, cond->sizeMMAP, PROT_READ|PROT_WRITE, MAP_ANONYMOUS, -1, 0);
		if (cond->mmap == MAP_FAILED) {
			goto cleanup;
		}

	}

	printf("sizeMMAP : %d\n",cond->sizeMMAP);

	cont = (struct content *) cond->mmap;
	cont->mutex = (pthread_mutex_t )PTHREAD_MUTEX_INITIALIZER;
	cont->condition = (pthread_cond_t )PTHREAD_COND_INITIALIZER;
	cont->var = 0;

	if(pthread_mutex_lock(&cont->mutex)){
		goto cleanup;
	}else{
		cont->isEmpty=1;
		cont->isEOF=0;
		cont->start=0;
		cont->end=0;
		cont->sizeMax=cond->c;
		cont->sizeAtom=cond->a;
		cont->sizeMMAP=cond->sizeMMAP;

		/*
		printf("CONT  -> %p\n",cont);
		printf("VAR   -> %p\n",&cont->var);
		printf("BUFF  -> %p\n",cont->buffCircular);
		*/

		int decalage=(sizeof(struct content));
		cont->buffCircular=(void *)cont + decalage ;

		//printf("BUFF2 -> %p\n",cont->buffCircular);

		/*
		for(int i=0;i<cond->c;i++ ){
			cont->buffCircular[i]='G';
		}
		*/

		if(pthread_mutex_unlock(&cont->mutex)){
			goto cleanup;
		}

		//msync(cond->mmap,cond->sizeMMAP,MS_SYNC);
	}

	//printf("FINI\n");
	return cond;

	cleanup:
		clean_Conduct(cond,FLAG_CLEAN_DESTROY);
		clean_Content(cont);
		return NULL;

}
struct conduct *conduct_open(const char *name) {


	if(name==NULL){
		return NULL;
	}

	struct conduct * cond = (struct conduct *) malloc(sizeof(struct conduct));
	struct content * cont=NULL;
	if (cond == NULL) {
		return NULL;
	}
	cond->mmap=MAP_FAILED;
	cond->modeMMAP = mode_NAMED;
	cond->fd = open(name, O_RDWR ,FILE_mode);

	if (cond->fd < 0) {
		goto cleanup;
	}

	struct stat sb;

	if (fstat(cond->fd, &sb) == -1){
		goto cleanup;
	}

	cond->sizeMMAP=sb.st_size;


	cond->mmap = mmap(NULL, cond->sizeMMAP/*TODO*/ , PROT_WRITE | PROT_READ,MAP_SHARED, cond->fd, 0);

	if (cond->mmap == MAP_FAILED) {
		goto cleanup;
	}

	cont = (struct content *) cond->mmap;

	cond->c = cont->sizeMax;
	cond->a = cont->sizeAtom;
	cond->sizeMMAP = cont->sizeMMAP;

	printf("SIZE MMAP DANS OPEN : %d\n",cond->sizeMMAP);


	return cond;


	cleanup:
		clean_Conduct(cond,FLAG_CLEAN_CLOSE);
		return NULL;



}
ssize_t conduct_read(struct conduct *c, void *buf, size_t count) {
	if (c == NULL || buf==NULL) {
		return -1;
	}

	char * localBuf =(char *)buf;

	struct content * ct=(struct content *)c->mmap;

	pthread_mutex_lock(&ct->mutex);

	while (ct->isEmpty) {

		if (ct->buffCircular[ct->start] == EOF) {
			pthread_mutex_unlock(&ct->mutex);
			return 0;
		} else {
			pthread_cond_wait(&ct->condition, &ct->mutex);
			continue;
		}

	}
	size_t sizeAvailable;
	ssize_t sizeReallyRead = 0;

	char passByMiddle=0;

	if (ct->start < ct->end) {
		sizeAvailable = ct->end - ct->start;

	} else {

		if (ct->start == ct->end) {
			if(ct->end!=c->c){
				passByMiddle=1;
			}
			sizeAvailable = c->c;
		}else{
			passByMiddle=1;
			sizeAvailable = c->c - ct->start + ct->end;
		}
	}


	size_t sizeToRead;
	if (count < sizeAvailable) {
		sizeToRead = count;
	} else {
		sizeToRead = sizeAvailable;
	}

	char tmp;
	size_t firstMaxFor;
	size_t secondMaxFor;
	if(passByMiddle && sizeToRead> c->c - ct->start){
		firstMaxFor=c->c;
	}else{
		firstMaxFor=ct->start + sizeToRead;
	}


	for (size_t i = ct->start; i < firstMaxFor; i++) {
		if ((tmp = ct->buffCircular[ct->start + i]) == EOF) {

		} else {
			localBuf[i] = ct->buffCircular[(ct->start) + i];
			sizeReallyRead++;
			ct->start++;
		}
	}

	if(passByMiddle){
		ct->start=0;
		for (size_t i = 0; i < secondMaxFor; i++) {
			if ((tmp = ct->buffCircular[ct->start + i]) == EOF) {

			} else {
				localBuf[i] = ct->buffCircular[(ct->start) + i];
				sizeReallyRead++;
				ct->start++;
			}
		}
	}


	pthread_mutex_unlock(&ct->mutex);
	return sizeReallyRead;


}
ssize_t conduct_write(struct conduct *c, const void *buf, size_t count) {
	if (c == NULL) {
		return -1;
	}

	//if (buf==NULL) {return -1;} FEATURE

	char * localBuf =(char *)buf;

	ssize_t  reallyWrite;

	struct content * ct=(struct content *)c->mmap;



	pthread_mutex_lock(&ct->mutex);
	if(ct->isEOF && count !=1 && localBuf[0]!=EOF){//TODO
		pthread_mutex_unlock(&ct->mutex);
		errno=EPIPE;
		return -1;
	}else if(count ==1 && localBuf[0]==EOF){
		while(ct->end==ct->start && !ct->isEmpty){
			pthread_cond_wait(&ct->condition, &ct->mutex);
		}
		ct->buffCircular[ct->end]=EOF;
		ct->end++;
		ct->isEOF=1;
		pthread_mutex_unlock(&ct->mutex);
		return count;
	}

	pthread_mutex_unlock(&ct->mutex);

	return  reallyWrite;

}
int conduct_write_eof(struct conduct *c) {
	if (c == NULL) {
		return -1;
	}

	char tmp=EOF;
	int result;
	size_t size=1;
	result = conduct_write(c,&tmp,size);
	if(result==-1 && errno==EPIPE){
		return 0;
	}
	return result;

}
void conduct_close(struct conduct *conduct) {
	clean_Conduct(conduct,FLAG_CLEAN_CLOSE);

	//TODO

}
void conduct_destroy(struct conduct *conduct) {
	clean_Conduct(conduct,FLAG_CLEAN_DESTROY);
	//TODO
}

