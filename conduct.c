/*
 *
 * Read , read from start to end
 *
 * Write , write from end to start
 *
 */

#include "conduct.h"
#include "conduct_aux.h"
#include "conduct_IO.h"

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

	if(msync(cond->mmap,cond->size_mmap,MS_SYNC)){
		goto cleanup;	
	}

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


ssize_t conduct_read(struct conduct *c, void *buf, size_t count) {

	struct iovec iov;
	iov.iov_base=(void *)buf;
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

	result = conduct_write_v_flag(c,&iov,1,FLAG_WRITE_NORMAL | flag);

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

