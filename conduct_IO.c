#include "conduct_IO.h"

extern inline ssize_t conduct_read_v_flag(struct conduct *c,const struct iovec *iov, int iovcnt,unsigned char flag){

	struct content * ct = (struct content *) c->mmap;
	struct dataCirularBuffer data = { 0 };



	if(init_dataCirularBuffer(&data,c,iov,iovcnt,flag)){
		printf("ERROR\n");
		return -1;
	}

	if ((flag & FLAG_O_NONBLOCK) != 0) {
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

			if ((flag & FLAG_O_NONBLOCK) != 0) {
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
	return data.sizeReallyManipulate;


}

extern inline ssize_t conduct_write_v_flag(struct conduct *c,const struct iovec *iov, int iovcnt,unsigned char flag){

	struct content * ct = (struct content *) c->mmap;
	struct dataCirularBuffer data = { 0 };

	if(init_dataCirularBuffer(&data,c,iov,iovcnt,flag)){
		printf("ERROR\n");
		return -1;
	}


	if ((flag & FLAG_O_NONBLOCK) != 0) {
		if(pthread_mutex_trylock(&ct->mutex)){
			return -1;
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
		return 0;
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

			if ((flag & FLAG_O_NONBLOCK) != 0) {
				errno=EAGAIN;
				return 0;
			}else{
				if (pthread_cond_wait(&ct->conditionWrite, &ct->mutex)) {
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


	return  data.sizeReallyManipulate;

}
