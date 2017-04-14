#ifndef CONDUCT_H_
#define CONDUCT_H

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>


struct conduct;

//API
struct conduct *conduct_create(const char *name, size_t a, size_t c);
struct conduct *conduct_open(const char *name);
ssize_t conduct_read(struct conduct *c, void *buf, size_t count);
ssize_t conduct_write(struct conduct *c, const void *buf, size_t count);
int conduct_write_eof(struct conduct *c);
void conduct_close(struct conduct *conduct);
void conduct_destroy(struct conduct *conduct);


//NON-BLOCKING API
ssize_t conduct_read_nb(struct conduct *c, void *buf, size_t count);
ssize_t conduct_write_nb(struct conduct *c, const void *buf, size_t count);
int conduct_write_eof_nb(struct conduct *c);


//VECTORED API
ssize_t conduct_writev(struct conduct *c,const struct iovec *iov, int iovcnt);
ssize_t conduct_readv(struct conduct *c,struct iovec *iov, int iovcnt);

//BONUS
int conduct_show(struct conduct *c);

#endif /* CONDUCT_H_ */
