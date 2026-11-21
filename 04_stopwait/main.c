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
		else {
			if (!s->quiet)
				printf("  [receiver] checksum FAILED -> NAK\n");
			reply = FRAME_NAK;
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
				printf("  [sender] NAK received -> fast retransmit\n");
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

int main(int argc, char **argv) {
	int	n_msgs	 = 5;
	int 	timeout  = 3;
	int	max_retx = 8;
	double	drop	 = 0.20;
	double	corrupt	 = 0.20;
	int 	seed 	 = 1;
	int	quiet	 = 0;

	int opt;
	while ((opt = getopt(argc, argv, "n:t:r:p:c:s:qh")) != -1) {
		switch (opt) {
			case 'n':
				n_msgs = atoi(optarg);
				break;
			case 't':
				timeout = atoi(optarg);
				break;
			case 'r':
				max_retx = atoi(optarg);
				break;
			case 'p':
				drop = atof(optarg);
				break;
			case 'c':
				corrupt = atof(optarg);
				break;
			case 's':
				seed = atoi(optarg);
				break;
			case 'q':
				quiet = 1;
				break;
			case 'h':
				usage(argv[0]);
				return 0;
			default:
				usage(argv[0]);
				return 1;
		}
	}

	if (n_msgs <= 0 || max_retx <= 0 || timeout <= 0 ||
	    drop < 0 || drop > 1 || corrupt < 0 ||
	    corrupt > 1) {
		fprintf(stderr, "Invalid parameters.\n");
		return 1;
	}

	srand((unsigned)seed);

	ChannelParams cp = {drop, corrupt};
	Sender s = {0, max_retx, quiet};
	Receiver r = {0, 0, 0};

	printf("===========================================\n");
	printf("   STOP-AND-WAIT ARQ over a noisy channel\n");
	printf("===========================================\n");
	printf("  Messages         : %d\n", n_msgs);
	printf("  Timeout (ticks)  : %d  (conceptual)\n", timeout);
	printf("  Max retx         : %d\n", max_retx);
	printf("  P(drop)          : %.2f\n", drop);
	printf("  P(corrupt)       : %.2f\n", corrupt);
	printf("  Seed             : %d\n", seed);

	int total_tx  = 0;
	int delivered = 0;
	int gave_up   = 0;

	for (int i = 0; i < n_msgs; ++i) {
		char msg[MAX_DATA];
		snprintf(msg, sizeof msg, "packet-%d", i);
		int attempts = sw_cycle(&s, &r, msg, &cp, &total_tx);
		if (attempts < 0) {
			printf("\n[RESULT] GAVE UP on \"%s\" after %d attempts\n", msg, max_retx);
			gave_up++;
		}
		else {
			printf("\n[RESULT] \"%s\" delivered in %d transmission(s)\n", msg, attempts);
			delivered++;
		}
	}

	printf("\n============ SUMMARY ============\n");
	printf("  Messages delivered  : %d / %d\n", delivered, n_msgs);
	printf("  Gave up             : %d\n", gave_up);
	printf("  Duplicates seen     : %d\n", r.duplicates);
	printf("  Total transmissions : %d\n", total_tx);
	printf("  Efficiency          : %.2f\n (delivered / total TX)", total_tx ? (double)delivered / (double) total_tx : 0.0);
	
	return gave_up ? 2 : 0;
}
			
