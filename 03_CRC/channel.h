#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include <stddef.h>

void channel_flip_bit(char *frame, size_t pos);

int channel_inject_random_errors(char *frame, int n);

#endif