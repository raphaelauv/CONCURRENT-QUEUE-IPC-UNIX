#include "conduct.h"

#define FILE_mode 0666

typedef struct custom_Mutex {
	int var;
	pthread_mutex_t mutex;
	pthread_cond_t condition;
} custom_Mutex;

struct content {
	custom_Mutex * cmutex;
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
		free(cont);
		cont = NULL;
	}

}

void clean_Conduct(struct conduct * cond) {
	if (cond != NULL) {
		if (cond->fd != -1) {
			close(cond->fd);
		}
		if (cond->mmap == (caddr_t) (-1)) {
			munmap(cond->mmap, cond->c);
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

	if (name == NULL) {

		cond->fd = shm_open("partage", O_CREAT | O_RDWR, FILE_mode);
		if (cond->fd < 0) {
			goto cleanup;
		}

		cond->mmap = mmap((caddr_t) 0, cond->c, PROT_WRITE | PROT_READ,
				MAP_SHARED, cond->fd, 0);
		if (cond->mmap == (caddr_t) (-1)) {
			goto cleanup;
		}

	} else {

	}

	cont = (struct content *) cond->mmap;

	cont->cmutex = (custom_Mutex *) malloc(sizeof(custom_Mutex));
	if (cont->cmutex == NULL) {
		goto cleanup;
	}
	init_custom_Mutex(cont->cmutex);

	//TODO

	cleanup:
		clean_Conduct(cond);
		clean_Content(cont);

	return NULL;

	return cond;

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

	ssize_t  reallyRead;

	//TODO

	return  reallyRead;

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
	if (conduct == NULL) {
		return;
	}

	//TODO

}
void conduct_destroy(struct conduct *conduct) {
	clean_Conduct(conduct);
	//TODO
}

