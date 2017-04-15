#ifndef CONDUCT_IO_H_
#define CONDUCT_IO_H_

#include "conduct.h"
#include "conduct_aux.h"

extern inline ssize_t conduct_read_v_flag(struct conduct *c,const struct iovec *iov, int iovcnt,unsigned char flag);
extern inline ssize_t conduct_write_v_flag(struct conduct *c,const struct iovec *iov, int iovcnt,unsigned char flag);

#endif /* CONDUCT_IO_H_ */
