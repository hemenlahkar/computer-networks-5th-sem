#ifndef __NET_H__
#define __NET_H__

#include <stddef.h>
#include <sys/types.h>

ssize_t read_all(int fd, void *buf, size_t n);

ssize_t write_all(int fd, const void *buf, size_t n);

void install_sigpipe_ignore(void);

#endif