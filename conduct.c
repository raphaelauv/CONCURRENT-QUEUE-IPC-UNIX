#include "conduct.h"

#define FILE_mode 0666

#define mode_ANONYMOUS 1
#define mode_NAMED 2

#define FLAG_CLEAN_DESTROY 1
#define FLAG_CLEAN_CLOSE 2

typedef struct custom_Mutex {
	int var;
	pthread_mutex_t mutex;
	pthread_cond_t condition;
} custom_Mutex;

struct content {
	char isEmpty;
	size_t start;
	size_t end;
	size_t max;
	custom_Mutex * cmutex;
	char * buffCircular;
};

int init_custom_Mutex(custom_Mutex * arg) {
	if (arg != NULL) {
		arg->mutex = (pthread_mutex_t )PTHREAD_MUTEX_INITIALIZER;
		arg->condition = (pthread_cond_t )PTHREAD_COND_INITIALIZER;
		arg->var = 0;
	} else {
		return -1;
	}
	return 0;
}

void clean_custom_Mutex(custom_Mutex * arg) {
	if (arg != NULL) {
		pthread_mutex_destroy(&arg->mutex);
		pthread_cond_destroy(&arg->condition);
		free(arg);
		arg = NULL;
	}
}

void clean_Content(struct content * cont) {
	if (cont != NULL) {
		clean_custom_Mutex(cont->cmutex);
		free(cont);
		cont = NULL;
	}
}

void clean_Conduct(struct conduct * cond,int flag) {
	if (cond != NULL) {


		if (cond->mmap != MAP_FAILED) {
			if(msync(cond->mmap,cond->c,MS_SYNC)){
				printf("ERROR msync()\n");
			}

			if(munmap(cond->mmap, cond->c)){
				printf("ERROR munmap()\n");
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

	struct content * cont;
	struct conduct * cond;

	cond = (struct conduct *) malloc(sizeof(struct conduct));
	if (cond == NULL) {
		return NULL;
	}

	cond->c = c;
	cond->a = a;
	cond->fd = -1;

	if (name != NULL) {

		cond->modeMMAP = mode_ANONYMOUS;
		cond->fd = shm_open(name, O_CREAT | O_RDWR | O_TRUNC, FILE_mode);

		if (cond->fd < 0) {
			goto cleanup;
		}

		struct stat sb;
		off_t offset, pa_offset;

		if (fstat(cond->fd, &sb) == -1){
			goto cleanup;
		}

		offset = cond->c;
		pa_offset = offset & ~(sysconf(_SC_PAGE_SIZE) - 1);

		if (offset >= sb.st_size) {
			if (ftruncate(cond->fd, c)) {
				goto cleanup;
			}
			//TODO
			close(cond->fd);
			cond->fd = shm_open(name, O_CREAT | O_RDWR | O_TRUNC, FILE_mode);
			if (cond->fd < 0) {
				goto cleanup;
			}
		}

		cond->mmap = mmap(NULL, cond->c /*TODO*/ , PROT_WRITE | PROT_READ,MAP_SHARED, cond->fd, 0);

		if (cond->mmap == MAP_FAILED) {
			goto cleanup;
		}

	} else {

		cond->modeMMAP = mode_NAMED;

		cond->mmap = mmap( NULL, cond->c , PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);

		if (cond->mmap == MAP_FAILED) {
			goto cleanup;
		}

	}

	cont = (struct content *) cond->mmap;

	cont->cmutex = (custom_Mutex *) malloc(sizeof(custom_Mutex));
	if (cont->cmutex == NULL) {
		goto cleanup;
	}
	init_custom_Mutex(cont->cmutex);

	pthread_mutex_lock(&cont->cmutex->mutex);
	cont->isEmpty=1;
	pthread_mutex_unlock(&cont->cmutex->mutex);

	//TODO

	return cond;

	cleanup:
		clean_Conduct(cond,FLAG_CLEAN_DESTROY);
		clean_Content(cont);
		return NULL;

}
struct conduct *conduct_open(const char *name) {

	struct conduct * cond;

	cond = (struct conduct *) malloc(sizeof(struct conduct));
	if (cond == NULL) {
		return NULL;
	}

	//TODO
	return cond;

}
ssize_t conduct_read(struct conduct *c, void *buf, size_t count) {
	if (c == NULL) {
		return -1;
	}

	char * localBuf =(char *)buf;

	struct content * ct=(struct content *)c->mmap;

	pthread_mutex_lock(&ct->cmutex->mutex);

	while (ct->isEmpty) {

		if (ct->buffCircular[ct->start] == EOF) {
			pthread_mutex_unlock(&ct->cmutex->mutex);
			return 0;
		} else {
			pthread_cond_wait(&ct->cmutex->condition, &ct->cmutex->mutex);
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
			if(ct->end!=ct->max){
				passByMiddle=1;
			}
			sizeAvailable = ct->max;
		}else{
			passByMiddle=1;
			sizeAvailable = ct->max - ct->start + ct->end;
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
	if(passByMiddle && sizeToRead> ct->max - ct->start){
		firstMaxFor=ct->max;
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


	pthread_mutex_unlock(&ct->cmutex->mutex);
	return sizeReallyRead;


}
ssize_t conduct_write(struct conduct *c, const void *buf, size_t count) {
	if (c == NULL) {
		return -1;
	}

	ssize_t  reallyWrite;

	//TODO

	return  reallyWrite;

}
int conduct_write_eof(struct conduct *c) {
	if (c == NULL) {
		return -1;
	}

	//TODO

	return 0;

}
void conduct_close(struct conduct *conduct) {
	clean_Conduct(conduct,FLAG_CLEAN_CLOSE);

	//TODO

}
void conduct_destroy(struct conduct *conduct) {
	clean_Conduct(conduct,FLAG_CLEAN_DESTROY);
	//TODO
}

