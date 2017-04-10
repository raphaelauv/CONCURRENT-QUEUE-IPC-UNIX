#ifndef CONDUCT_H_
#define CONDUCT_H

#include <stdlib.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h> // for open
#include <sys/file.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/stat.h> /* For mode constants */
#include <fcntl.h> /* For O_* constants */

#include <pthread.h>


#define MALERROR

struct content;

struct conduct{
	size_t c;
	size_t a;
	int fd;
	void * mmap;
	void *content;
};

struct conduct *conduct_create(const char *name, size_t a, size_t c);
struct conduct *conduct_open(const char *name);
ssize_t conduct_read(struct conduct *c, void *buf, size_t count);
ssize_t conduct_write(struct conduct *c, const void *buf, size_t count);
int conduct_write_eof(struct conduct *c);
void conduct_close(struct conduct *conduct);
void conduct_destroy(struct conduct *conduct);

#endif /* CONDUCT_H_ */
