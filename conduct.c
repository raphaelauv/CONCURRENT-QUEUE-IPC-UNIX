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

#define FLAG_WRITE 1
#define FLAG_READ 1

#define MAXIMUM_SIZE_NAME_CONDUCT 100

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
	int fd;
	int size_mmap;
	char * fileName;
	void * mmap;
};

struct content {
	int var;
	pthread_mutex_t mutex;
	pthread_cond_t condition;

	int size_mmap;
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

			if(msync(cond->mmap,cond->size_mmap,MS_SYNC)){
				printf("ERROR msync()\n");
				error=1;
			}

			if(flag==FLAG_CLEAN_DESTROY){
				if (munmap(cond->mmap, cond->size_mmap)) {
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


		if(cond->fileName!=NULL){
			if (flag == FLAG_CLEAN_DESTROY) {
				if (unlink(cond->fileName)) {
					error = 1;
				}
			}
			//free(cond->fileName); //TODO
			cond->fileName=NULL;
		}

		free(cond);
		cond = NULL;
	}
	return error;
}

extern inline int copyFileName(const char * fileName ,struct conduct * cond){

	int i=0;
	while(i<MAXIMUM_SIZE_NAME_CONDUCT && fileName[i]!='\0' && fileName[i]!=EOF){
		cond->fileName[i]=fileName[i];
		i++;
	}

	if(i==MAXIMUM_SIZE_NAME_CONDUCT){
		return -1;
	}

	return 0;
}

struct conduct *conduct_create(const char *name, size_t a, size_t c) {

	struct content * cont = NULL;
	struct conduct * cond = NULL;

	cond = (struct conduct *) malloc(sizeof(struct conduct));

	if (cond == NULL) {
		errno = EINVAL;
		return NULL;
	}

	cond->fd = -1;
	cond->fileName = NULL;
	cond->mmap = MAP_FAILED;
	cond->size_mmap = c;
	cond->size_mmap += sizeof(struct content);

	if (name != NULL) {
		cond->fileName = malloc(sizeof(char) * MAXIMUM_SIZE_NAME_CONDUCT);
		if(copyFileName(name,cond)){
			errno = ENAMETOOLONG;
			goto cleanup;
		}

		cond->fd = open(name,O_CREAT | O_RDWR | O_TRUNC,FILE_mode);

		if (cond->fd < 0) {
			goto cleanup;
		}

		struct stat sb;
		off_t offset, pa_offset;

		if (fstat(cond->fd, &sb) == -1){
			goto cleanup;
		}


		offset = cond->size_mmap;
		//pa_offset = offset & ~(sysconf(_SC_PAGE_SIZE) - 1);

		if (offset >= sb.st_size) {
			if (ftruncate(cond->fd, cond->size_mmap)) {
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

		cond->mmap = mmap(NULL, cond->size_mmap/*TODO*/ , PROT_WRITE | PROT_READ,MAP_SHARED, cond->fd, 0);

		if (cond->mmap == MAP_FAILED) {
			goto cleanup;
		}

	} else {


		cond->mmap = mmap( NULL, cond->size_mmap, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
		if (cond->mmap == MAP_FAILED) {
			goto cleanup;
		}

	}

	printf("sizeMMAP : %d\n",cond->size_mmap);

	cont = (struct content *) cond->mmap;
	cont->mutex = (pthread_mutex_t )PTHREAD_MUTEX_INITIALIZER;
	cont->condition = (pthread_cond_t )PTHREAD_COND_INITIALIZER;
	cont->var = 0;

	if(pthread_mutex_lock(&cont->mutex)){
		errno = ENOLCK;
		goto cleanup;
	}else{
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

	cond->fd = open(name, O_RDWR ,FILE_mode);
	if (cond->fd < 0) {
		goto cleanup;
	}

	struct stat sb;
	if (fstat(cond->fd, &sb) == -1){
		goto cleanup;
	}

	cond->size_mmap=sb.st_size;
	cond->mmap = mmap(NULL, cond->size_mmap/*TODO*/ , PROT_WRITE | PROT_READ,MAP_SHARED, cond->fd, 0);
	if (cond->mmap == MAP_FAILED) {
		goto cleanup;
	}

	cont = (struct content *) cond->mmap;

	if (pthread_mutex_lock(&cont->mutex)) {
		errno=ENOLCK;
		goto cleanup;
	} else {
		cond->size_mmap = cont->size_mmap;

		if (pthread_mutex_unlock(&cont->mutex)) {
			goto cleanup;
		}
	}

	printf("SIZE MMAP DANS OPEN : %d\n",cond->size_mmap);

	return cond;


	cleanup:
		clean_Conduct(cond,FLAG_CLEAN_CLOSE);
		return NULL;



}

extern inline void eval_limit_loops(struct content * ct,struct dataCirularBuffer * data,int flag){

	if(data->passByMiddle){
		data->firstMaxFor=ct->sizeMax;

		int restToPerform=data->sizeToManipulate;

		if(flag==FLAG_WRITE){
			restToPerform-= (ct->sizeMax - ct->end);

		}else{
			restToPerform-= (ct->sizeMax - ct->start);
		}

		if( restToPerform > 0){
			data->secondMaxFor=restToPerform;
		}

	}else{

		if(flag==FLAG_WRITE){
			data->firstMaxFor=ct->end;
		}else{
			data->firstMaxFor=ct->start;
		}
		data->firstMaxFor+=data->sizeToManipulate;

	}

}

extern inline void eval_size_to_manipulate(struct content * ct,struct dataCirularBuffer * data,int flag) {


	if (data->count >ct->sizeAtom) {
		data->sizeToManipulate = ct->sizeAtom;

		if(flag==FLAG_WRITE){
			return;
		}
	}else{
		if(flag==FLAG_WRITE){
			data->sizeToManipulate = data->count;
			return;
		}
		data->sizeToManipulate=data->count;

	}

	if(data->sizeToManipulate>data->sizeAvailable){
		data->sizeToManipulate = data->sizeAvailable;
	}

}

extern inline void eval_position_and_size_of_data(struct content * ct,struct dataCirularBuffer * data,int flag) {

	if(ct->isEmpty){

		if(flag==FLAG_WRITE){
			data->sizeAvailable = ct->sizeMax;
			if(ct->start!=0){
				data->passByMiddle=1;
			}

		}else{
			data->sizeAvailable = 0;
		}

		return;

	}else{
		if (ct->start == ct->end) {

			if (flag == FLAG_WRITE) {
				data->sizeAvailable = 0;
			} else {
				data->sizeAvailable = ct->sizeMax;
			}

			if (ct->start != 0) {
				data->passByMiddle = 1;

			}
			return;
		}
	}

	if (ct->start < ct->end) {

		if(flag==FLAG_READ){
			data->sizeAvailable = ct->end - ct->start;
		}else {
			data->sizeAvailable = (ct->sizeMax - ct->end) + ct->start;
			data->passByMiddle = 1;
		}

		return;

	}else{

		if (flag == FLAG_READ) {
			data->sizeAvailable = ct->sizeMax - (ct->start - ct->end);
			data->passByMiddle = 1;
		}else{
			data->sizeAvailable = ct->start -ct->end;
		}

		return;
	}
}



ssize_t conduct_read(struct conduct *c, void *buf, size_t count) {
	if (c == NULL || buf==NULL) {
		errno=EINVAL;
		return -1;
	}

	char * localBuf = (char *) buf;
	struct content * ct = (struct content *) c->mmap;
	struct dataCirularBuffer data = { 0 };
	size_t i;
	data.count = count;

	if(pthread_mutex_lock(&ct->mutex)){
		errno=ENOLCK;
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

	eval_position_and_size_of_data(ct,&data,FLAG_READ);
	while(data.sizeAvailable<1){
		pthread_cond_wait(&ct->condition, &ct->mutex);
		eval_position_and_size_of_data(ct,&data,FLAG_READ);
	}

	printf("in read passbyMille = %d\n",data.passByMiddle);


	//eval_size_to_manipulate(ct,&data,FLAG_READ); TODO


	if (count >ct->sizeAtom) {
		data.sizeToManipulate = ct->sizeAtom;
	}else{
		data.sizeToManipulate=count;
	}

	if(data.sizeToManipulate>data.sizeAvailable){
		data.sizeToManipulate = data.sizeAvailable;
	}

	//eval_limit_loops(ct,&data,FLAG_READ); TODO

	if(data.passByMiddle){
		data.firstMaxFor=ct->sizeMax;
		int restToRead=data.sizeToManipulate - (ct->sizeMax - ct->start);

		if( restToRead > 0){
			data.secondMaxFor=restToRead;
		}

	}else{
		data.firstMaxFor=ct->start + data.sizeToManipulate;
	}

	printf("firstMaxFor READ : %d\n",(int)data.firstMaxFor);
	printf("secondMaxFor READ : %d\n",(int)data.secondMaxFor);

	int EOFfind=0;
	char tmp;
	for (i = ct->start; i < data.firstMaxFor; i++) {
		tmp = ct->buffCircular[i];
		if (tmp == EOF) {
			EOFfind=1;
			break;
		} else {
			localBuf[data.sizeReallyManipulate] = tmp;
			data.sizeReallyManipulate++;
			ct->start++;
		}
	}

	if(data.passByMiddle && !EOFfind){
		ct->start=0;
		for (i = 0; i < data.secondMaxFor; i++) {
			tmp = ct->buffCircular[i];
			if (tmp == EOF) {
				EOFfind=1;
				break;
			} else {
				localBuf[data.sizeReallyManipulate] =tmp;
				data.sizeReallyManipulate++;
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
	return data.sizeReallyManipulate;


}
ssize_t conduct_write(struct conduct *c, const void *buf, size_t count) {
	if (c == NULL || count==0) {
		errno=EINVAL;
		return -1;
	}

	//if (buf==NULL) {return -1;} FEATURE

	char * localBuf = (char *) buf;
	struct content * ct = (struct content *) c->mmap;
	struct dataCirularBuffer data = { 0 };
	size_t i = 0;
	data.count = count;

	if(pthread_mutex_lock(&ct->mutex)){
		errno=ENOLCK;
		return -1;
	}

	if (ct->isEOF) {
		pthread_mutex_unlock(&ct->mutex);
		errno = EPIPE;
		return -1;
	}

	eval_size_to_manipulate(ct,&data,FLAG_WRITE);

	eval_position_and_size_of_data(ct,&data,FLAG_WRITE);

	while((ct->end==ct->start && !ct->isEmpty) || data.sizeToManipulate>data.sizeAvailable){
		//buffer is FULL for the moment or there is not sufisant place
		if(pthread_cond_wait(&ct->condition, &ct->mutex)){
			return -1;
		}

		if (ct->isEOF) {
			pthread_mutex_unlock(&ct->mutex);
			errno = EPIPE;
			return -1;
		}

		eval_position_and_size_of_data(ct,&data,FLAG_WRITE);
	}

	if (count == 1 && localBuf[0] == EOF) {
		ct->buffCircular[ct->end] = EOF;
		ct->end++;
		ct->isEOF = 1;
		pthread_mutex_unlock(&ct->mutex);
		return count;
	}

	printf("in write passbyMille = %d\n",data.passByMiddle);

	eval_limit_loops(ct,&data,FLAG_WRITE);

/*
	if(data.passByMiddle){
		data.firstMaxFor=c->c;

		int restToWrite=data.sizeToManipulate - (c->c - ct->end);

		if( restToWrite > 0){
			data.secondMaxFor=restToWrite;
		}

	}else{
		data.firstMaxFor=ct->end + data.sizeToManipulate;
	}
*/
	printf("firstMaxFor WRITE : %d\n",(int)data.firstMaxFor);
	printf("secondMaxFor WRITE : %d\n",(int)data.secondMaxFor);

	for ( i = ct->end; i < data.firstMaxFor; i++) {

		ct->buffCircular[i]=localBuf[data.sizeReallyManipulate];
		data.sizeReallyManipulate++;
		ct->end++;
	}

	if(data.passByMiddle){
		if(ct->end!=ct->sizeMax){
			printf("ERROR DEPLACEMENT FIN BUFFER CIRCULAIRE\n");
		}

		ct->end=0;
		for ( i = 0; i < data.secondMaxFor; i++) {
			ct->buffCircular[i]=localBuf[data.sizeReallyManipulate];
			data.sizeReallyManipulate++;
			ct->end++;
		}
	}

	if(data.sizeReallyManipulate>0){
		ct->isEmpty=0;
	}

	pthread_cond_signal(&ct->condition);
	pthread_mutex_unlock(&ct->mutex);

	return  data.sizeReallyManipulate;

}

int conduct_write_eof(struct conduct *c) {
	if (c == NULL) {
		errno = EINVAL;
		return -1;
	}

	char tmp = EOF;//TODO
	int result;
	size_t size=1;
	result = conduct_write(c,&tmp,size);
	if( (result==-1 && errno==EPIPE)  || result==1){
		return 0;//EOF is already write , or just been write
	}
	return result;

}
void conduct_close(struct conduct *conduct) {
	clean_Conduct(conduct, FLAG_CLEAN_CLOSE);
}
void conduct_destroy(struct conduct *conduct) {
	clean_Conduct(conduct, FLAG_CLEAN_DESTROY);
}

