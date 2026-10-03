#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "stopwait.h"
#include "channel.h"

typedef struct {
	int next_seq;
	int max_retx;
	int quiet;
} Sender;

typedef struct {
	int expected_seq;
	int delivered;
	int duplicates;
} Receiver;

static int sw_cycle(Sender *s, Receiver *r, const char *msg, const ChannelParams *cp, int *total_tx) {
	Frame f, ack;
	int seq = s->next_seq;

	for (int attempt = 1; attempt <= s->max_retx; ++attempt) {
		if (!s->quiet)
			printf("\n[TX #%d] Sender transmits seq=%d\n", attempt, seq);
		sw_frame_init_data(&f, (uint8_t)seq, msg, strlen(msg));
		(*total_tx)++;

		if (!channel_transmit(cp, &f, "frame")) {
			if (!s->quiet) printf("  [sender]  timeout - retransmitting\n");
			continue;
		}

		int frame_ok = sw_frame_is_valid(&f);
		int reply;

		if (frame_ok) {
			if (f.seq == r->expected_seq) {
				if (!s->quiet)
					printf("  [receiver] frame OK, seq=%d -> DELIVER \"%.*s\"\n", f.seq, (int)f.len, f.data);
				r->expected_seq ^= 1;
				r->delivered++;
				reply = FRAME_ACK;
			} else {
				if (!s->quiet)
					printf("  [receiver] checsum FAILED -> NAK\n");
				reply = FRAME_NAK;
			}
		}

		sw_frame_init_ctrl(&ack, reply, (uint8_t)seq);
		if (!channel_transmit(cp, &ack, (reply == FRAME_ACK) ? "ACK" : "NAK")) {
			if (!s->quiet) printf("  [sender]  timeout - ACK/NAK lost\n");
			continue;
		}
		if (!sw_frame_is_valid(&ack) || ack.seq != seq) {
			if (!s->quiet)
				printf("  [sender]  bad/stale control frame -> ignore, timout\n");
			continue;
		}
		if (ack.kind == FRAME_NAK) {
			if (!s->quiet)
				pritnf("  [sender] NAK received -> fast retransmit\n");
			continue;
		}
		if (!s->quiet) printf("  [sender] ACK seq=%d received\n", ack.seq);
		s->next_seq ^= 1;
		return attempt;
	}
	return -1;
}


static void usage(const char *prog) {
	fprintf(stderr,
			"Usage: %s [options]\n"
			"\n"
			"Stop-and-Wait ARQ simulation over a noisy channel.\n"
			"\n"
			"Options:\n"
			"  -n <msgs>     number of messages to send        (default 5)\n"
			"  -t <ticks>    sender timeout (informational)    (default 3)\n"
			"  -r <retx>     max retransmissions per frame     (default 8)\n"
			"  -p <prob>     P(frame lost) in channel          (default 0.20)\n"
			"  -c <prob>     P(bit error) in channel           (default 0.20)\n"
			"  -s <seed>     RNG seed                          (default 1)\n"
			"  -q            quiet: suppress per-event log\n"
			"  -h            show this help\n"
			"\n"
			"Exit codes: 0 = all messages delivered, 1 = bad arguements, 2 = gave up\n",
			prog);
}

