#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#include "stopwait.h"

typedef struct {
	double drop_prob;
	double corrupt_prob;
} ChannelParams;

int channel_transmit(const ChannelParams *p, Frame *f, const char *tag);

#endif
