#include "conduct_aux.h"

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

extern inline int init_dataCirularBuffer(struct dataCirularBuffer * data,struct conduct *c,const struct iovec *iov, int iovcnt,unsigned char flag){

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

		//for iter at end
		data->currentBuf=iov[0].iov_base;
		data->currentCount=iov[0].iov_len;


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

	int currentIndexIOV=0;
	int currentIter=0;
	int allCurrentCounts=data->currentCount;

	char modeWrite=0;

	if(flag==INTERNAL_FLAG_READ){
		modeWrite=1;
	}


	limit=data->firstMaxFor;

	for(k=0;k<1+data->passByMiddle;k++){

		if(k==1){

			if(modeWrite){
				ct->start=0;
			}else{
				if (ct->end != ct->sizeMax) {
					printf("ERROR DEPLACEMENT FIN BUFFER CIRCULAIRE\n");
				}
				ct->end=0;
			}

			limit=data->secondMaxFor;
		}
		if(modeWrite){
			i=ct->start;
		}else{
			i=ct->end;
		}
		for ( ; i < limit; i++) {

			if (data->sizeReallyManipulate == allCurrentCounts) {
				currentIter = 0;
				currentIndexIOV++;
				data->currentBuf = iov[currentIndexIOV].iov_base;
				data->currentCount = iov[currentIndexIOV].iov_len;
				allCurrentCounts += data->currentCount;
			}

			if(modeWrite){
				data->currentBuf[currentIter]=ct->buffCircular[i];
				ct->start++;
			}else{
				ct->buffCircular[i]=data->currentBuf[currentIter];
				ct->end++;
			}

			currentIter++;
			data->sizeReallyManipulate++;

		}

	}
}
