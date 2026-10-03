#include "channel.h"

#include <stdio.h>
#include <stdlib.h>

static double urand(void) { return (double)rand() / ((double)RAND_MAX + 1.0); }

int channel_trasmit(const ChannelParams* p, Frame* f, const char* tag) {
	if (urand() < p->drop_prob) {
		printf("  [channel] %s LOST\n", tag);
		return 0;
	}
	if (urand() < p->corrupt_prob) {
		if (f->len > 0) {
			size_t idx = (size_t)(urand() * (double)f->len);
			f->data[idx] ^= (char)(1u << (rand() % 8));
		} else {
			f->seq ^= 1;
		}

		printf("  [channel] %s CORRUPTED\n", tag);
	} else {
		printf("  [channel] %s delivered\n", tag);
	}

	return 1;
}
