#include "conduct.h"

#define FILE_mode 0666
#define mode_ANONYMOUS 1
#define mode_NAMED 2
#define FLAG_CLEAN_DESTROY 1
#define FLAG_CLEAN_CLOSE 2

#define FLAG_WRITE 1
#define FLAG_READ 1

struct conduct{
	char modeMMAP;
	char * fileName;
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



extern inline void clean_Content(struct content * cont) {
	if (cont != NULL) {
		pthread_mutex_destroy(&cont->mutex);
		pthread_cond_destroy(&cont->condition);
	}
}

extern inline int clean_Conduct(struct conduct * cond,int flag) {

	int error=0;
	if (cond != NULL) {

		if (cond->mmap != MAP_FAILED) {

			if(msync(cond->mmap,cond->mmap,MS_SYNC)){
				printf("ERROR msync()\n");
				error=1;
			}

			if(flag==FLAG_CLEAN_DESTROY){
				if (munmap(cond->mmap, cond->c)) {
					error=1;
					printf("ERROR munmap()\n");
				}
			}

		}
		if (cond->fd != -1) {
			if(close(cond->fd)){
				error=1;
			}
		}

		if (flag == FLAG_CLEAN_DESTROY && cond->modeMMAP==mode_NAMED) {
			if (unlink(cond->fileName)) {
				error=1;
			}
		}
		free(cond);
		cond = NULL;
	}
	return error;
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
			close(cond->fd);
			cond->fd = open(name, O_RDWR | O_TRUNC,FILE_mode);
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
		cond->mmap = mmap( NULL, cond->sizeMMAP, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
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

	struct content * cont = NULL;
	struct conduct * cond = (struct conduct *) malloc(sizeof(struct conduct));
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

	if (pthread_mutex_lock(&cont->mutex)) {
		goto cleanup;
	} else {
		cond->c = cont->sizeMax;
		cond->a = cont->sizeAtom;
		cond->sizeMMAP = cont->sizeMMAP;

		if (pthread_mutex_unlock(&cont->mutex)) {
			goto cleanup;
		}
	}

	printf("SIZE MMAP DANS OPEN : %d\n",cond->sizeMMAP);

	return cond;


	cleanup:
		clean_Conduct(cond,FLAG_CLEAN_CLOSE);
		return NULL;



}

extern inline void evalDATAinBUFF(struct content * ct,size_t *sizeAvailable, char * passByMiddle,int flag) {

	if(ct->isEmpty){

		if(flag==FLAG_WRITE){
			*sizeAvailable = ct->sizeMax;
		}else{
			*sizeAvailable = 0;
		}
		return;
	}else{
		if (ct->start == ct->end) {

			if (flag == FLAG_WRITE) {
				*sizeAvailable = 0;
			} else {
				*sizeAvailable = ct->sizeMax;
			}

			if (ct->start != 0) {
				*passByMiddle = 1;

			}
			return;
		}
	}

	if (ct->start < ct->end) {

		if(flag==FLAG_READ){
			*sizeAvailable = ct->end - ct->start;
		}else {
			*sizeAvailable = (ct->sizeMax - ct->end) + ct->start;
			*passByMiddle = 1;
		}

		return;

	}else{

		if (flag == FLAG_READ) {
			*sizeAvailable = ct->sizeMax - (ct->start - ct->end);
			*passByMiddle = 1;
		}else{
			*sizeAvailable = ct->start -ct->end;
		}

		return;
	}
}


ssize_t conduct_read(struct conduct *c, void *buf, size_t count) {
	if (c == NULL || buf==NULL) {
		return -1;
	}

	char * localBuf =(char *)buf;

	struct content * ct=(struct content *)c->mmap;

	size_t sizeAvailable = 0;
	char passByMiddle = 0;
	size_t firstMaxFor = 0;
	size_t secondMaxFor = 0;
	ssize_t sizeReallyRead = 0;
	size_t sizeToRead = 0;
	size_t i = 0;

	if(pthread_mutex_lock(&ct->mutex)){
		return -1;
	}

	while (ct->isEmpty) {
		if (ct->buffCircular[ct->start] == EOF) {
			pthread_mutex_unlock(&ct->mutex);
			return 0;
		} else {
			pthread_cond_wait(&ct->condition, &ct->mutex);
		}
	}

	evalDATAinBUFF(ct,&sizeAvailable,&passByMiddle,FLAG_READ);
	while(sizeAvailable<1){
		pthread_cond_wait(&ct->condition, &ct->mutex);
		evalDATAinBUFF(ct,&sizeAvailable,&passByMiddle,FLAG_READ);
	}

	printf("in read passbyMille = %d\n",passByMiddle);


	if (count >c->a) {
		sizeToRead = c->a;
	}else{
		sizeToRead=count;
	}

	if(sizeToRead>sizeAvailable){
		sizeToRead = sizeAvailable;
	}


	if(passByMiddle && (ct->start+sizeToRead )>ct->sizeMax){
		firstMaxFor=ct->sizeMax;
		secondMaxFor= sizeToRead - (ct->sizeMax - ct->start);
	}else{
		firstMaxFor=ct->start + sizeToRead;
	}

	int EOFfind=0;
	char tmp;
	for (i = ct->start; i < firstMaxFor; i++) {
		tmp = ct->buffCircular[i];
		if (tmp == EOF) {
			EOFfind=1;
			break;
		} else {
			localBuf[sizeReallyRead] = tmp;
			sizeReallyRead++;
			ct->start++;
		}
	}

	if(passByMiddle && !EOFfind){
		ct->start=0;
		for (i = 0; i < secondMaxFor; i++) {
			tmp = ct->buffCircular[i];
			if (tmp == EOF) {
				EOFfind=1;
				break;
			} else {
				localBuf[sizeReallyRead] =tmp;
				sizeReallyRead++;
				ct->start++;
			}
		}
	}

	if(ct->start==ct->end){
		ct->isEmpty=1;
	}

	if(EOFfind){
		ct->isEmpty=1;
	}

	pthread_cond_signal(&ct->condition);
	pthread_mutex_unlock(&ct->mutex);
	return sizeReallyRead;


}
ssize_t conduct_write(struct conduct *c, const void *buf, size_t count) {
	if (c == NULL || count==0) {
		return -1;
	}

	//if (buf==NULL) {return -1;} FEATURE

	char * localBuf =(char *)buf;

	struct content * ct=(struct content *)c->mmap;

	size_t sizeToWrite = 0;
	size_t sizeAvailable = 0;
	char passByMiddle = 0;
	size_t firstMaxFor = 0;
	size_t secondMaxFor = 0;
	ssize_t sizeReallyWrite = 0;
	size_t i = 0;

	if(pthread_mutex_lock(&ct->mutex)){
		//TODO
		return -1;
	}

	if (ct->isEOF) {
		pthread_mutex_unlock(&ct->mutex);
		errno = EPIPE;
		return -1;
	}

	if(count>c->a){
		sizeToWrite=c->a;
	}else{
		sizeToWrite=count;
	}

	evalDATAinBUFF(ct,&sizeAvailable,&passByMiddle,FLAG_WRITE);

	while((ct->end==ct->start && !ct->isEmpty) || sizeToWrite>sizeAvailable){
		//buffer is FULL for the moment
		//or there is not sufisant place
		if(pthread_cond_wait(&ct->condition, &ct->mutex)){
			return -1;
		}

		if (ct->isEOF) {
			pthread_mutex_unlock(&ct->mutex);
			errno = EPIPE;
			return -1;
		}

		evalDATAinBUFF(ct,&sizeAvailable,&passByMiddle,FLAG_WRITE);
	}

	if (count == 1 && localBuf[0] == EOF) {
		ct->buffCircular[ct->end] = EOF;
		ct->end++;
		ct->isEOF = 1;
		pthread_mutex_unlock(&ct->mutex);
		return count;
	}

	printf("in write passbyMille = %d\n",passByMiddle);


	if(passByMiddle && (sizeToWrite > (c->c - ct->start))){
		firstMaxFor=c->c;
	}else{
		firstMaxFor=ct->start + sizeToWrite;
	}

	for ( i = ct->start; i < firstMaxFor; i++) {

		ct->buffCircular[i]=localBuf[sizeReallyWrite];
		sizeReallyWrite++;
		ct->end++;
	}

	if(passByMiddle){
		if(ct->end!=ct->sizeMax){
			printf("ERROR DEPLACEMENT FIN BUFFER CIRCULAIRE\n");
		}

		ct->end=0;
		for ( i = 0; i < secondMaxFor; i++) {
			ct->buffCircular[i]=localBuf[sizeReallyWrite];
			sizeReallyWrite++;
			ct->end++;
		}
	}

	if(sizeReallyWrite>0){
		ct->isEmpty=0;
	}

	pthread_cond_signal(&ct->condition);
	pthread_mutex_unlock(&ct->mutex);

	return  sizeReallyWrite;

}

int conduct_write_eof(struct conduct *c) {
	if (c == NULL) {
		return -1;
	}

	char tmp=EOF;
	int result;
	size_t size=1;
	result = conduct_write(c,&tmp,size);
	if(result==-1 && errno==EPIPE  || result==1){
		return 0;//EOF is already write , or just been write
	}
	return result;

}
void conduct_close(struct conduct *conduct) {
	clean_Conduct(conduct,FLAG_CLEAN_CLOSE);
}
void conduct_destroy(struct conduct *conduct) {
	clean_Conduct(conduct,FLAG_CLEAN_DESTROY);
}

