/*
 * Read , read from start to end
 * Write , write from end to start
 */

#include "conduct.h"

#define MODE_MEMCPY 0						// TRUE -> use memcpy standard C library function else it don't

#define FILE_mode 0666						// FILE mode opening
#define FLAG_CLEAN_DESTROY 1
#define FLAG_CLEAN_CLOSE 2
#define INTERNAL_FLAG_WRITE 1
#define INTERNAL_FLAG_READ 2
#define FLAG_WRITE_EOF 4
#define FLAG_WRITE_NORMAL 8
#define FLAG_O_NONBLOCK 16

#define MAXIMUM_SIZE_NAME_CONDUCT 100		// Size Max for the file's name
#define LIMIT_SHOW "——————————————————————"


//all the informations necessary during a conduct operation
struct dataCirularBuffer{
	char passByMiddle;						// Boolean , TRUE if the read or write operation is going to pass from the end to the start of the buffer
	size_t count;							// Size the User want to read or write on the CircularBuffer
	size_t sizeAvailable;					// Size available to read or write on the CircularBuffer
	size_t firstLimitLoop;					// Value of the limit to read or write on the CircularBuffer
	size_t secondLimitLoop;					// only when passByMiddle is TRUE
	size_t sizeToManipulate;				// Size we going to read or write on the CircularBuffer with the actual mutex take
	ssize_t sizeReallyManipulate;			// Size total read or write on the CircularBuffer with all the hypothetical mutex took

	//indices for the loop read or write
	char * current_iov_base;				// Pointer to the current IOV array
	size_t current_iov_len;					// Size of the current IOV array
	size_t allcurrent_iov_len;				// Addition of all array's sizes of IOV already seen and the current own
	int currentIndexIOV;					// Index of the current Array in the IOV structure
	int currentIterIOV;						// Index of current position in the current array of IOV structure

};

struct conduct{
	size_t size_mmap;						// Size of the MAPED MEMORY
	char * fileName;						// NULL or Name of the file maped
	void * mmap;							// Pointer on the start of MAPED MEMORY
};


//all the content store inside the maped memory
struct content {
	pthread_mutex_t mutex;					// MUTEX of the CircularBuffer
	pthread_cond_t conditionRead;			// For sending signals to readers waiting
	pthread_cond_t conditionWrite;			// For sending signals to writers waiting

	size_t size_mmap;						// Size maped ( necessary for  conduct_open function )
	size_t sizeMax;							// Size of the CircularBuffer
	size_t sizeAtom;						// Guaranteed atomic limit
	size_t start;							// Index of the position from where readers read
	size_t end;								// Index of the position from where writers write

	char isEOF;								// Boolean , TRUE -> EOF have been insert in the CircularBuffer
	char isEmpty;							// Boolean , TRUE -> the CircualarBuffer is Empty and so start == end
	char *buffCircular;						// Pointer on the CircularBuffer
};


/*
 *  Print the content of the CircularBuffer and show below the position of start and end indices
 */
int conduct_show(struct conduct *c){
	struct content * ct = (struct content *) c->mmap;

	char array[(ct->sizeMax) + 1];
	char array2[(ct->sizeMax) + 1];

	for(int i=0; i<ct->sizeMax;i++){
		array2[i]=' ';
	}

	array[(ct->sizeMax)]='\0';
	array2[(ct->sizeMax)]='\0';

	size_t start=0;
	size_t end=0;
	char isEOF=0;
	char isEmpty=0;

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

extern inline int init_Content(struct content * cont) {
	int result=0;

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

	result+=pthread_mutexattr_destroy(&mutexAttr);
	result+=pthread_condattr_destroy(&condAttrRead);
	result+=pthread_condattr_destroy(&condAttrWrite);

	return result;

}


extern inline int clean_Conduct(struct conduct * cond,int flag) {

	int error=0;
	if (cond == NULL) {
		errno = EINVAL;
		error=1;
	}else{

		if (cond->mmap != MAP_FAILED) {

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
			}else{
				if (msync(cond->mmap, cond->size_mmap, MS_SYNC)) {
					printf("ERROR msync()\n");
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

	if (close(fd)) {
		//TODO
	}
	fd=-1;

	cont = (struct content *) cond->mmap;

	if(init_Content(cont)){
		goto cleanup;
	}

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

	msync(cond->mmap,cond->size_mmap,MS_SYNC);//TODO test return value and MS_INVALIDATE

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

	if (close(fd)) {
		//TODO
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
		data->firstLimitLoop=ct->sizeMax;

		int restToPerform=data->sizeToManipulate;

		if(flag==INTERNAL_FLAG_WRITE){
			restToPerform-= (ct->sizeMax - ct->end);

		}else{
			restToPerform-= (ct->sizeMax - ct->start);
		}

		if( restToPerform > 0){
			data->secondLimitLoop=restToPerform;
		}

	}else{

		if(flag==INTERNAL_FLAG_WRITE){
			data->firstLimitLoop=ct->end ;
		}else{
			data->firstLimitLoop=ct->start;
		}
		data->firstLimitLoop += data->sizeToManipulate;

	}

}

extern inline void eval_size_to_manipulate(struct content * ct,struct dataCirularBuffer * data) {
	if (data->count - data->sizeReallyManipulate >ct->sizeAtom) { // sizeTodo - sizeAlreadyDO > atomic guarented ?
		data->sizeToManipulate = ct->sizeAtom;
	}else{
		data->sizeToManipulate=data->count - data->sizeReallyManipulate;
	}
}

extern inline void eval_position_and_size_of_data(struct content * ct,struct dataCirularBuffer * data,int flag) {

	if(ct->isEmpty){ //and so ct.start == ct.end

		if(flag==INTERNAL_FLAG_WRITE){
			data->sizeAvailable = ct->sizeMax;

			if ( (ct->end + data->sizeToManipulate) >= ct->sizeMax) {
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

				if ((ct->start + data->sizeToManipulate) >= ct->sizeMax) {
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

	}else{ //ct->start > ct-> end

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

extern inline int init_dataCirularBuffer(struct dataCirularBuffer * data,struct conduct *c,const struct iovec *iov, int iovcnt){

	if (c == NULL || iovcnt == 0 || iov == NULL) {
			errno = EINVAL;
			return -1;
		}
		const char * buf=iov[0].iov_base;
		const int count=iov[0].iov_len;

		if (buf == NULL || count == 0) {
			errno = EINVAL;
			return -1;
		}

		data->current_iov_base=iov[0].iov_base;
		data->current_iov_len=iov[0].iov_len;
		data->allcurrent_iov_len=data->current_iov_len;
		//printf("ALL CURRENT IOVLEN %d\n",data->allcurrent_iov_len);

		data->currentIndexIOV=0;
		data->currentIterIOV=0;
		size_t sizeTotal=0;

		for (int i = 0; i < iovcnt; i++) {
			sizeTotal += iov[i].iov_len;
		}

		data->count = sizeTotal;

		return 0;
}

extern inline void apply_loops(struct dataCirularBuffer * data,struct content *ct,const struct iovec *iov,unsigned char flag){
	int k=0;
	int limit;
	size_t i;
	char modeRead=0;

	if(flag==INTERNAL_FLAG_READ){
		modeRead=1;
	}

	limit=data->firstLimitLoop;
	//printf("LIMIT 1 -> %d\n",limit);

	for(k=0;k<1+data->passByMiddle;k++){

		if(k==1){

			if(modeRead){
				ct->start=0;
			}else{
				if (ct->end != ct->sizeMax) {
					printf("ERROR DEPLACEMENT FIN BUFFER CIRCULAIRE\n");
				}
				ct->end=0;
			}

			limit=data->secondLimitLoop;
			//printf("LIMIT 2 -> %d\n",limit);
		}
		if(modeRead){
			i=ct->start;
		}else{
			i=ct->end;
		}

		#if MODE_MEMCPY
		while(i<limit){
		#else//NO MEMCPY VERSION
		for ( ; i < limit; i++) {
		#endif
			if (data->sizeReallyManipulate == data->allcurrent_iov_len) {
				//printf("CHANGEMENT IOV | sizeManipulate %d\n",data->sizeReallyManipulate);
				data->currentIterIOV = 0;
				data->currentIndexIOV++;
				data->current_iov_base = iov[data->currentIndexIOV].iov_base;
				data->current_iov_len = iov[data->currentIndexIOV].iov_len;
				data->allcurrent_iov_len += data->current_iov_len;
			}else{
				//printf("PAS CHANGEMENT IOV | sizeManipulate %d\n",data->sizeReallyManipulate);
			}

			#if MODE_MEMCPY
			int sizeLimit = data->current_iov_len - data->currentIterIOV; // The limit is the number of boxes available inside the current IOV array

			if (sizeLimit > data->sizeToManipulate) { // To not exceed the number of boxes available to use
				sizeLimit = data->sizeToManipulate;
			}
			if(i+sizeLimit>limit){ // To not exceed the size of the CircularBuffer
				sizeLimit=limit -i;
			}
			#endif

			if(modeRead){
				#if MODE_MEMCPY
				memcpy( &(data->current_iov_base[data->currentIterIOV]),  &(ct->buffCircular[i]),sizeLimit);

				ct->start+=sizeLimit;
				#else //NO MEMCPY VERSION
				data->current_iov_base[data->currentIterIOV]=ct->buffCircular[i];
				ct->start++;
				#endif
			}else{
				#if MODE_MEMCPY
				memcpy( &(ct->buffCircular[i]), &(data->current_iov_base[data->currentIterIOV]),sizeLimit);
				ct->end+=sizeLimit;
				#else //NO MEMCPY VERSION
				ct->buffCircular[i]=data->current_iov_base[data->currentIterIOV];
				ct->end++;
				#endif
			}

			#if MODE_MEMCPY
			data->currentIterIOV+=sizeLimit;
			i+=sizeLimit;
			data->sizeReallyManipulate+=sizeLimit;
			#else //NO MEMCPY VERSION
			data->currentIterIOV++;
			data->sizeReallyManipulate++;
			#endif

		}
		//printf("END LOOP | sizeManipulate %d\n",data->sizeReallyManipulate);

	}
}


extern inline ssize_t conduct_read_v_flag(struct conduct *c,const struct iovec *iov, int iovcnt,unsigned char flag){

	int retry=0;

	struct content * ct = (struct content *) c->mmap;

	
	if (atomic_load(&(ct->isEOF))) {
		errno = EPIPE;
		return -1;
	}

	struct dataCirularBuffer data = { 0 };
	if(init_dataCirularBuffer(&data,c,iov,iovcnt)){
		//TODO errno
		return -1;
	}

	if ((flag & FLAG_O_NONBLOCK) != 0) {
		if(pthread_mutex_trylock(&ct->mutex)){
			return 0;
		}
	}else{
		if (pthread_mutex_lock(&ct->mutex)) {
			return -1;
		}
	}


retry_it:

	if(retry){
		if(pthread_mutex_trylock(&ct->mutex)){
			errno=0;
			return data.sizeReallyManipulate;
		}
	}

	int needReEval=0;

	do{
		if(ct->isEOF) {
			if(pthread_mutex_unlock(&ct->mutex)){
				return -1;
			}
			errno = EPIPE;
			return -1;
		}

		eval_size_to_manipulate(ct,&data);
		eval_position_and_size_of_data(ct,&data,INTERNAL_FLAG_READ);
		if (data.sizeToManipulate > data.sizeAvailable) {
			data.sizeToManipulate = data.sizeAvailable;
			eval_position_and_size_of_data(ct,&data,INTERNAL_FLAG_READ);
		}

		if (ct->isEmpty) {

			if (retry) {
				if (pthread_mutex_unlock(&ct->mutex)) {
					return -1;
				}
				errno = 0;
				return data.sizeReallyManipulate;
			}


			if ((flag & FLAG_O_NONBLOCK) != 0) {
				if(pthread_mutex_unlock(&ct->mutex)){
					return -1;
				}
				errno=EWOULDBLOCK;
				return 0;
			}else{
				if (pthread_cond_wait(&ct->conditionRead, &ct->mutex)) {
					pthread_mutex_unlock(&ct->mutex);
					return -1;
				}
				needReEval=1;
			}

		}else{
			needReEval=0;
		}

	}while(ct->isEOF || ct->isEmpty || needReEval);

	eval_limit_loops(ct,&data,INTERNAL_FLAG_READ);

	//printf("READ passByMiddle : %d | for1 : %d | for2: %d\n",data.passByMiddle,(int)data.firstMaxFor,(int)data.secondMaxFor);

	apply_loops(&data,ct,iov,INTERNAL_FLAG_READ);

	if (ct->start == ct->end) {
		ct->isEmpty=1;
	}

	if(pthread_cond_signal(&ct->conditionWrite)){
		return -1;
	}
	if(pthread_mutex_unlock(&ct->mutex)){
		return -1;
	}

	if (data.sizeReallyManipulate < data.count) {
		//printf("SIZE ALREADY READ : %d\n",data.sizeReallyManipulate);
		//printf("TAILLE %d , contenu : %s\n",iov->iov_len,iov->iov_base);
		retry = 1;
		data.passByMiddle=0;// Reset value
		goto retry_it;
	}

	return data.sizeReallyManipulate;

}

extern inline ssize_t conduct_write_v_flag(struct conduct *c,const struct iovec *iov, int iovcnt,unsigned char flag){

	int retry=0;

	struct content * ct = (struct content *) c->mmap;

	/*
	if ((flag & FLAG_WRITE_EOF) !=0 ) {
		atomic_store(&(ct->isEOF),1);
		return 0;
	}
	*/

	if (atomic_load(&(ct->isEOF))) {
		errno = EPIPE;
		return -1;
	}


	struct dataCirularBuffer data = { 0 };

	if(init_dataCirularBuffer(&data,c,iov,iovcnt)){
		return -1;
	}


	if ((flag & FLAG_O_NONBLOCK) != 0) {
		if(pthread_mutex_trylock(&ct->mutex)){
			errno=EWOULDBLOCK;
			return -1;
		}
	}else{
		if (pthread_mutex_lock(&ct->mutex)) {
			return -1;
		}
	}

retry_it:

	if(retry){
		//conduct_show(c);
		if(pthread_mutex_trylock(&ct->mutex)){
			errno=0;
			return data.sizeReallyManipulate;
		}
		//printf("RETRY pour %d\n",data.count);
	}
	
	if ((flag & FLAG_WRITE_EOF) !=0 ) {
		ct->isEOF = 1;
		pthread_mutex_unlock(&ct->mutex);
		return 0;
	}

	do{
		if (ct->isEOF) {
			pthread_mutex_unlock(&ct->mutex);
			errno = EPIPE;
			return -1;
		}

		eval_size_to_manipulate(ct,&data);
		//printf("SIZE TO MANIPULATE %d\n",data.sizeToManipulate);
		eval_position_and_size_of_data(ct,&data,INTERNAL_FLAG_WRITE);

		if((ct->end==ct->start && !ct->isEmpty) || data.sizeToManipulate>data.sizeAvailable){
			//buffer is FULL for the moment or there is not sufisant place

			if (retry) {
				if (pthread_mutex_unlock(&ct->mutex)) {
					return -1;
				}
				errno = 0;
				return data.sizeReallyManipulate;
			}

			if ((flag & FLAG_O_NONBLOCK) != 0) {
				if(pthread_mutex_unlock(&ct->mutex)){
					return -1;
				}
				errno=EWOULDBLOCK;
				return 0;
			}else{
				if (pthread_cond_wait(&ct->conditionWrite, &ct->mutex)) {
					pthread_mutex_unlock(&ct->mutex);
					return -1;
				}
			}


		}

	}while(ct->isEOF || (ct->end==ct->start && !ct->isEmpty) || data.sizeToManipulate>data.sizeAvailable);

	eval_limit_loops(ct,&data,INTERNAL_FLAG_WRITE);

	//printf("WRITE passByMiddle : %d | for1 : %d | for2: %d\n",data.passByMiddle,(int)data.firstMaxFor,(int)data.secondMaxFor);

	apply_loops(&data,ct,iov,INTERNAL_FLAG_WRITE);

	if(data.sizeReallyManipulate>0){
		ct->isEmpty=0;
	}

	if(pthread_cond_signal(&ct->conditionRead)){
		return -1;
	}
	if(pthread_mutex_unlock(&ct->mutex)){
		return -1;
	}

	if (data.sizeReallyManipulate < data.count) {
		retry = 1;
		data.passByMiddle=0; // Reset value
		goto retry_it;
	}

	return  data.sizeReallyManipulate;

}

ssize_t conduct_read(struct conduct *c, void *buf, size_t count) {

	struct iovec iov;
	iov.iov_base= buf;
	iov.iov_len=count;

	return conduct_read_v_flag(c,&iov,1,0);
}

ssize_t conduct_write(struct conduct *c, const void *buf, size_t count) {

	struct iovec iov;
	iov.iov_base=(void *)buf;
	iov.iov_len=count;

	return conduct_write_v_flag(c,&iov,1,FLAG_WRITE_NORMAL);
}

int conduct_write_eof_FLAG(struct conduct *c,unsigned char flag) {

	int result;
	char tmp[1];
	size_t size = 1;

	struct iovec iov;
	iov.iov_base=(void *)tmp;
	iov.iov_len=size;

	result = conduct_write_v_flag(c,&iov,1,FLAG_WRITE_EOF | flag);

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
	struct iovec iov;
	iov.iov_base=(void *)buf;
	iov.iov_len=count;

	return conduct_read_v_flag(c,&iov,1,FLAG_O_NONBLOCK);
}
ssize_t conduct_write_nb(struct conduct *c, const void *buf, size_t count){
	struct iovec iov;
	iov.iov_base=(void *)buf;
	iov.iov_len=count;

	return conduct_write_v_flag(c,&iov,1,FLAG_WRITE_NORMAL|FLAG_O_NONBLOCK);
}
int conduct_write_eof_nb(struct conduct *c){
	return conduct_write_eof_FLAG(c,FLAG_O_NONBLOCK);
}

ssize_t conduct_writev(struct conduct *c,const struct iovec *iov, int iovcnt){
	return conduct_write_v_flag(c,iov,iovcnt,FLAG_WRITE_NORMAL);
}
ssize_t conduct_readv(struct conduct *c,struct iovec *iov, int iovcnt){
	return conduct_read_v_flag(c,iov,iovcnt,0);
}

