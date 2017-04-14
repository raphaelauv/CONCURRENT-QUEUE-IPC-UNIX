/*
 *
 * Read , read from start to end
 *
 * Write , write from end to start
 *
 */

#include "conduct.h"

#define FILE_mode 0666

#define FLAG_CLEAN_DESTROY 1
#define FLAG_CLEAN_CLOSE 2

#define INTERNAL_FLAG_WRITE 1
#define INTERNAL_FLAG_READ 2

#define FLAG_WRITE_EOF 1
#define FLAG_WRITE_NORMAL 2
#define FLAG_NON_BLOCKING 4

#define MAXIMUM_SIZE_NAME_CONDUCT 100
#define LIMIT_SHOW "————————————————————————"

struct dataCirularBuffer{
	char passByMiddle;
	size_t count;
	size_t sizeAvailable;
	size_t firstMaxFor;
	size_t secondMaxFor;
	size_t sizeToManipulate;
	ssize_t sizeReallyManipulate;
};

struct conduct{
	int size_mmap;
	char * fileName;
	void * mmap;
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

int conduct_show(struct conduct *c){
	struct content * ct = (struct content *) c->mmap;

	char array[(ct->sizeMax) + 1];
	char array2[(ct->sizeMax) + 1];

	for(int i=0; i<ct->sizeMax;i++){
		array2[i]=' ';
	}

	array[(ct->sizeMax)]='\0';
	array2[(ct->sizeMax)]='\0';

	int start=0;
	int end=0;
	int isEOF=0;
	int isEmpty=0;

	if(pthread_mutex_lock(&ct->mutex)){
		return -1;
	}

	isEOF = ct->isEOF;
	isEmpty=ct->isEmpty;
	start=ct->start;
	end=ct->end;

	for(int i=0;i<ct->sizeMax;i++){
		array[i]=ct->buffCircular[i];

		if(array[i]==0){
			array[i]='-';
		}
		if(array[i]=='\n' || array[i]=='\r'){
			array[i]='&';
		}
	}

	if(pthread_mutex_unlock(&ct->mutex)){
		return -1;
	}

	if(start==end){
		array2[start]='#';
	}else{
		array2[start]='S';
		array2[end]='E';
	}

	int newJump=0;

	printf("\n"LIMIT_SHOW"\n%s\n%s\n",array,array2);
	if(isEmpty){
		printf("BUFFER EMPTY ");
		newJump=1;
	} else {
		if (start == end) {
			printf("BUFFER FULL ");
			newJump=1;
		}

	}

	if(isEOF){
		printf(" EOF");
		newJump=1;
	}
	if(newJump){
		printf("\n");
	}
	printf(LIMIT_SHOW"\n\n");

	return 0;
}

extern inline void clean_Content(struct content * cont) {
	if (cont != NULL) {
		pthread_cond_destroy(&cont->conditionRead);
		pthread_cond_destroy(&cont->conditionWrite);
		pthread_mutex_destroy(&cont->mutex);
	}
}

extern inline int clean_Conduct(struct conduct * cond,int flag) {

	int error=0;
	if (cond == NULL) {
		errno = EINVAL;
		error=1;
	}else{

		if (cond->mmap != MAP_FAILED) {

			if(cond->fileName!=NULL){
				if (msync(cond->mmap, cond->size_mmap, MS_SYNC)) {
					printf("ERROR msync()\n");
					error = 1;
				}
			}

			if(flag==FLAG_CLEAN_DESTROY){
				struct content * cont = (struct content *) cond->mmap;
				clean_Content(cont);
			}

			if (munmap(cond->mmap, cond->size_mmap)) {
				printf("ERROR munmap()\n");
				error = 1;
			}

			cond->mmap=NULL;

		}

		if(cond->fileName!=NULL){
			if (flag == FLAG_CLEAN_DESTROY) {

				if (unlink(cond->fileName)) {
					error = 1;
				}
			}
			free(cond->fileName);
			cond->fileName=NULL;
		}
		free(cond);
	}

	return error;
}

extern inline int copyFileName(const char * fileName ,struct conduct * cond){

	cond->fileName = malloc(sizeof(char) * MAXIMUM_SIZE_NAME_CONDUCT);
	int i=0;
	while(i<MAXIMUM_SIZE_NAME_CONDUCT && fileName[i]!='\0' && fileName[i]!=EOF){
		cond->fileName[i]=fileName[i];
		i++;
	}

	cond->fileName[i]='\0';

	//printf("NOM COPIER : %s\n",cond->fileName);

	if(i==MAXIMUM_SIZE_NAME_CONDUCT){
		return -1;
	}

	return 0;
}

struct conduct *conduct_create(const char *name, size_t a, size_t c) {

	if(c<1 || a<1){
		errno=EINVAL;
		return NULL;
	}

	int result=0;

	struct content * cont = NULL;
	struct conduct * cond = NULL;

	cond = (struct conduct *) malloc(sizeof(struct conduct));

	if (cond == NULL) {
		errno = EINVAL;
		return NULL;
	}

	cond->fileName = NULL;
	cond->mmap = MAP_FAILED;
	cond->size_mmap = c + sizeof(struct content);

	int fd=-1;

	if (name != NULL) {
		if(copyFileName(name,cond)){
			errno = ENAMETOOLONG;
			goto cleanup;
		}

		fd = open(name,O_CREAT | O_RDWR | O_TRUNC,FILE_mode);

		if (fd < 0) {
			goto cleanup;
		}

		struct stat sb;

		if (fstat(fd, &sb) == -1){
			goto cleanup;
		}

		if (cond->size_mmap >= sb.st_size) {
			if (ftruncate(fd, cond->size_mmap)) {
				goto cleanup;
			}

			/*
			close(fd);
			fd=-1;
			fd = open(name, O_RDWR | O_TRUNC,FILE_mode);
			if (fd < 0) {
				goto cleanup;
			}
			*/

		}

		cond->mmap = mmap(NULL, cond->size_mmap , PROT_WRITE | PROT_READ,MAP_SHARED, fd, 0);

		if (cond->mmap == MAP_FAILED) {
			goto cleanup;
		}

	} else {


		cond->mmap = mmap( NULL, cond->size_mmap, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
		if (cond->mmap == MAP_FAILED) {
			goto cleanup;
		}

	}
	if (fd != -1) {
		if (close(fd)) {
		}
	}

	cont = (struct content *) cond->mmap;

	result=0;

	pthread_mutexattr_t mutexAttr;
	pthread_condattr_t condAttrRead;
	pthread_condattr_t condAttrWrite;

	result+=pthread_condattr_init(&condAttrRead);
	result+=pthread_condattr_setpshared(&condAttrRead,PTHREAD_PROCESS_SHARED);
	result+=pthread_cond_init(&cont->conditionRead, &condAttrRead);

	result+=pthread_condattr_init(&condAttrWrite);
	result+=pthread_condattr_setpshared(&condAttrWrite,PTHREAD_PROCESS_SHARED);
	result+=pthread_cond_init(&cont->conditionWrite, &condAttrWrite);

	result+=pthread_mutexattr_init(&mutexAttr);
	result+=pthread_mutexattr_setpshared(&mutexAttr, PTHREAD_PROCESS_SHARED);
	result+=pthread_mutex_init(&cont->mutex, &mutexAttr);

	if(result){
		goto cleanup;
	}

	pthread_mutexattr_destroy(&mutexAttr);
	pthread_condattr_destroy(&condAttrRead);
	pthread_condattr_destroy(&condAttrWrite);


	if(pthread_mutex_lock(&cont->mutex)){
		goto cleanup;
	}

	cont->isEmpty=1;
	cont->isEOF=0;
	cont->start=0;
	cont->end=0;
	cont->sizeMax=c;
	cont->sizeAtom=a;
	cont->size_mmap=cond->size_mmap;

	/*
	printf("CONT  -> %p\n",cont);
	printf("VAR   -> %p\n",&cont->var);
	printf("BUFF  -> %p\n",cont->buffCircular);
	*/

	int decalage=(sizeof(struct content));
	cont->buffCircular=(void *)cont + decalage ;

	if(pthread_mutex_unlock(&cont->mutex)){
		goto cleanup;
	}

	msync(cond->mmap,cond->size_mmap,MS_SYNC);

	return cond;

	cleanup:
		if (fd != -1) {
			if (close(fd)) {
			}
		}
		clean_Conduct(cond,FLAG_CLEAN_DESTROY);
		return NULL;

}

struct conduct *conduct_open(const char *name) {

	if(name==NULL){
		errno=EINVAL;
		return NULL;
	}

	struct content * cont = NULL;
	struct conduct * cond = (struct conduct *) malloc(sizeof(struct conduct));
	if (cond == NULL) {
		return NULL;
	}

	int fd=-1;

	if (copyFileName(name, cond)) {
		errno = ENAMETOOLONG;
		goto cleanup;
	}

	cond->mmap=MAP_FAILED;

	fd = open(name, O_RDWR ,FILE_mode);
	if (fd < 0) {
		goto cleanup;
	}

	struct stat sb;
	if (fstat(fd, &sb) == -1){
		goto cleanup;
	}

	cond->size_mmap=sb.st_size;

	cond->mmap = mmap(NULL, cond->size_mmap , PROT_WRITE | PROT_READ,MAP_SHARED, fd, 0);
	if (cond->mmap == MAP_FAILED) {
		goto cleanup;
	}
	if (fd != -1) {
		if (close(fd)) {
		}
	}

	cont = (struct content *) cond->mmap;

	if (pthread_mutex_lock(&cont->mutex)){
		goto cleanup;
	}

	cond->size_mmap = cont->size_mmap;

	if (pthread_mutex_unlock(&cont->mutex)) {
		goto cleanup;
	}

	//printf("SIZE MMAP DANS OPEN : %d\n",cond->size_mmap);
	return cond;


	cleanup:
		if (fd != -1) {
			if (close(fd)) {
			}
		}

		clean_Conduct(cond,FLAG_CLEAN_CLOSE);
		return NULL;
}

extern inline void eval_limit_loops(struct content * ct,struct dataCirularBuffer * data,int flag){

	if(data->passByMiddle){
		data->firstMaxFor=ct->sizeMax;

		int restToPerform=data->sizeToManipulate;

		if(flag==INTERNAL_FLAG_WRITE){
			restToPerform-= (ct->sizeMax - ct->end);

		}else{
			restToPerform-= (ct->sizeMax - ct->start);
		}

		if( restToPerform > 0){
			data->secondMaxFor=restToPerform;
		}

	}else{

		if(flag==INTERNAL_FLAG_WRITE){
			data->firstMaxFor=ct->end ;
		}else{
			data->firstMaxFor=ct->start;
		}
		data->firstMaxFor += data->sizeToManipulate;

	}

}

extern inline void eval_size_to_manipulate(struct content * ct,struct dataCirularBuffer * data,int flag) {


	if (data->count >ct->sizeAtom) {
		data->sizeToManipulate = ct->sizeAtom;

		if(flag==INTERNAL_FLAG_WRITE){
			return;
		}
	}else{
		if(flag==INTERNAL_FLAG_WRITE){
			data->sizeToManipulate = data->count;
			return;
		}
		data->sizeToManipulate=data->count;

	}
}

extern inline void eval_position_and_size_of_data(struct content * ct,struct dataCirularBuffer * data,int flag) {

	if(ct->isEmpty){

		if(flag==INTERNAL_FLAG_WRITE){
			data->sizeAvailable = ct->sizeMax;

			if ( (ct->end + data->sizeToManipulate) > ct->sizeMax) {
				data->passByMiddle = 1;
			}

		}else{
			data->sizeAvailable = 0;
		}

		return;

	}else{
		if (ct->start == ct->end) {

			if (flag == INTERNAL_FLAG_WRITE) {
				data->sizeAvailable =0;

			} else {
				data->sizeAvailable = ct->sizeMax;

				if ((ct->start + data->sizeToManipulate) > ct->sizeMax) {
					data->passByMiddle = 1;
				}

			}


			return;
		}
	}

	if (ct->start < ct->end) {

		if(flag==INTERNAL_FLAG_READ){
			data->sizeAvailable = ct->end - ct->start;
			data->passByMiddle=0;
		}else {
			data->sizeAvailable = (ct->sizeMax - ct->end) + ct->start;

			if(ct->sizeMax - ct->end <= data->sizeToManipulate){
				data->passByMiddle=1;
			}
		}

		return;

	}else{

		if (flag == INTERNAL_FLAG_READ) {
			data->sizeAvailable = (ct->sizeMax - ct->start) + ct->end;

			if(ct->sizeMax - ct->start <= data->sizeToManipulate){
				data->passByMiddle = 1;
			}

		}else{
			data->sizeAvailable = ct->start -ct->end;
		}

		return;
	}
}

extern inline ssize_t conduct_read_FLAG(struct conduct *c, const void *buf, size_t count,unsigned char flag) {

	if (c == NULL || buf==NULL || count==0) {
			errno=EINVAL;
			return -1;
		}

		char * localBuf = (char *) buf;
		struct content * ct = (struct content *) c->mmap;
		struct dataCirularBuffer data = { 0 };
		data.count = count;

		//for iter at end
			int k;
			int limit;
			size_t i;


		if ((flag & FLAG_NON_BLOCKING) != 0) {
			if(pthread_mutex_trylock(&ct->mutex)){
				return 0;
			}
		}else{
			if (pthread_mutex_lock(&ct->mutex)) {
				//errno=ENOLCK;
				return -1;
			}
		}

		int needReEval=0;

		do{
			if(ct->isEOF) {
				if(pthread_mutex_unlock(&ct->mutex)){
					return -1;
				}
				return 0;
			}

			eval_size_to_manipulate(ct,&data,INTERNAL_FLAG_READ);
			eval_position_and_size_of_data(ct,&data,INTERNAL_FLAG_READ);
			if (data.sizeToManipulate > data.sizeAvailable) {
				data.sizeToManipulate = data.sizeAvailable;
			}
			eval_position_and_size_of_data(ct,&data,INTERNAL_FLAG_READ);
			if (data.sizeToManipulate > data.sizeAvailable) {
				data.sizeToManipulate = data.sizeAvailable;
			}

			//printf("in READ EVAL DONE\n");

			if (ct->isEmpty) {
				//printf("in READ WAIT\n");

				if ((flag & FLAG_NON_BLOCKING) != 0) {
					errno=EAGAIN;
					return 0;
				}else{
					if (pthread_cond_wait(&ct->conditionRead, &ct->mutex)) {
						return -1;
					}
					needReEval=1;
				}

				//printf("in READ OUT OF  WAIT\n");
			}else{
				needReEval=0;
			}

		}while(ct->isEOF || ct->isEmpty || needReEval);

		eval_limit_loops(ct,&data,INTERNAL_FLAG_READ);

		//printf("READ passByMiddle : %d | for1 : %d | for2: %d\n",data.passByMiddle,(int)data.firstMaxFor,(int)data.secondMaxFor);

		limit=data.firstMaxFor;

		for(k=0;k<1+data.passByMiddle;k++){

			if(k==1){
				ct->start=0;
				limit=data.secondMaxFor;
			}

			for (i = ct->start; i < limit; i++) {
				localBuf[data.sizeReallyManipulate] = ct->buffCircular[i];
				data.sizeReallyManipulate++;
				ct->start++;
			}

		}

		if (ct->start == ct->end) {
			ct->isEmpty=1;
		}

		if(pthread_cond_signal(&ct->conditionWrite)){
			return -1;
		}
		if(pthread_mutex_unlock(&ct->mutex)){
			return -1;
		}
		return data.sizeReallyManipulate;


}

ssize_t conduct_read(struct conduct *c, void *buf, size_t count) {

	return conduct_read_FLAG(c,buf,count,0);

}


extern inline ssize_t conduct_write_FLAG(struct conduct *c, const void *buf, size_t count,unsigned char flag) {

	if (c == NULL || count==0) {
		errno=EINVAL;
		return -1;
	}

	//if (buf==NULL) {errno=EINVAL;return -1;} FEATURE

	char * localBuf = (char *) buf;
	struct content * ct = (struct content *) c->mmap;
	struct dataCirularBuffer data = { 0 };
	data.count = count;

	//for iter at end
		int k;
		int limit;
		size_t i;


	if ((flag & FLAG_NON_BLOCKING) != 0) {
		if(pthread_mutex_trylock(&ct->mutex)){
			return count;
		}
	}else{
		if (pthread_mutex_lock(&ct->mutex)) {
			//errno=ENOLCK;
			return -1;
		}
	}

	if ((flag & FLAG_WRITE_EOF) !=0 ) {
		ct->isEOF = 1;
		pthread_mutex_unlock(&ct->mutex);
		return count;
	}


	do{
		if (ct->isEOF) {
			pthread_mutex_unlock(&ct->mutex);
			errno = EPIPE;
			return -1;
		}

		eval_size_to_manipulate(ct,&data,INTERNAL_FLAG_WRITE);
		eval_position_and_size_of_data(ct,&data,INTERNAL_FLAG_WRITE);

		if((ct->end==ct->start && !ct->isEmpty) || data.sizeToManipulate>data.sizeAvailable){
			//buffer is FULL for the moment or there is not sufisant place

			if ((flag & FLAG_NON_BLOCKING) != 0) {
				errno=EAGAIN;
				return count;
			}else{
				if (pthread_cond_wait(&ct->conditionWrite, &ct->mutex)) {
					return -1;
				}
			}


		}

	}while(ct->isEOF || (ct->end==ct->start && !ct->isEmpty) || data.sizeToManipulate>data.sizeAvailable);

	eval_limit_loops(ct,&data,INTERNAL_FLAG_WRITE);

	//printf("WRITE passByMiddle : %d | for1 : %d | for2: %d\n",data.passByMiddle,(int)data.firstMaxFor,(int)data.secondMaxFor);


	limit=data.firstMaxFor;

	for(k=0;k<1+data.passByMiddle;k++){
		if(k==1){
			if (ct->end != ct->sizeMax) {
				printf("ERROR DEPLACEMENT FIN BUFFER CIRCULAIRE\n");
			}
			ct->end=0;
			limit=data.secondMaxFor;
		}

		for ( i = ct->end; i < limit; i++) {
			ct->buffCircular[i]=localBuf[data.sizeReallyManipulate];
			data.sizeReallyManipulate++;
			ct->end++;
		}
	}

	if(data.sizeReallyManipulate>0){
		ct->isEmpty=0;
	}

	if(pthread_cond_signal(&ct->conditionRead)){
		return -1;
	}
	if(pthread_mutex_unlock(&ct->mutex)){
		return -1;
	}

	return  data.sizeReallyManipulate;



}

ssize_t conduct_write(struct conduct *c, const void *buf, size_t count) {
	return conduct_write_FLAG(c,buf,count,FLAG_WRITE_NORMAL);
}


int conduct_write_eof_FLAG(struct conduct *c,unsigned char flag) {

	int result;
	char tmp[1];
	size_t size = 1;
	result = conduct_write_FLAG(c, &tmp, size, FLAG_WRITE_EOF | flag);
	if ((result == -1 && errno == EPIPE) || result == 1) {
		return 0; //EOF is already write , or just been write
	}
	return result;
}

int conduct_write_eof(struct conduct *c) {
	return conduct_write_eof_FLAG(c,0);
}

void conduct_close(struct conduct *conduct) {
	clean_Conduct(conduct, FLAG_CLEAN_CLOSE);
}


void conduct_destroy(struct conduct *conduct) {
	clean_Conduct(conduct, FLAG_CLEAN_DESTROY);
}



ssize_t conduct_read_nb(struct conduct *c, void *buf, size_t count){
	return conduct_read_FLAG(c,buf,count,FLAG_NON_BLOCKING);
}
ssize_t conduct_write_nb(struct conduct *c, const void *buf, size_t count){
	return conduct_write_FLAG(c,buf,count,FLAG_WRITE_NORMAL|FLAG_NON_BLOCKING);
}
int conduct_write_eof_nb(struct conduct *c){
	return conduct_write_eof_FLAG(c,FLAG_NON_BLOCKING);
}


ssize_t conduct_writev(struct conduct *c,const struct iovec *iov, int iovcnt){
	return 0;
}
ssize_t conduct_readv(struct conduct *c,struct iovec *iov, int iovcnt){
	return 0;
}

