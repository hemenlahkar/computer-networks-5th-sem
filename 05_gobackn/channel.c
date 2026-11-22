// channel.c
#include "channel.h"

#include <stdio.h>
#include <stdlib.h>

static double urand(void) { return (double)rand() / ((double)RAND_MAX + 1.0); }

int channel_transmit(const ChannelParams* p, Frame* f, const char* tag) {
	if (urand() < p->drop_prob) {
		printf("   [channel] %s DROPPED\n", tag);
		return 0;
	}
	if (urand() < p->corrupt_prob) {
		if (f->len > 0) {
			size_t idx = (size_t)(urand() * (double)f->len);
			f->data[idx] ^= (char)(1u << (rand() % 8));
		} else {
			f->seq = (f->seq + 1) % SEQ_MOD;
		}
		printf("   [channel] %s CORRUPTED\n", tag);
	} else {
		printf("   [channel] %s delivered\n", tag);
	}

	return 1;
}

