// main.c
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "channel.h"
#include "gobackn.h"

#define MAX_MSGS 64
#define MAX_BURSTS 64

static void usage(const char* prog) {
	fprintf(
	    stderr,
	    "Usage: %s [options]\n"
	    "\n"
	    "Go-Back-N ARQ simulation over a noisy channel.\n"
	    "\n"
	    "Options:\n"
	    "  -n <msgs>    number of messages to send        (default 8)\n"
	    "  -w <size>    window size (must be < %d)         (default 4)\n"
	    "  -t <rounds>  timout in rounds                  (default 3)\n"
	    "  -m <rounds>  max simulation rounds             (default 200)\n"
	    "  -p <prob>    P(frame drop) in channel          (default 0.20)\n"
	    "  -c <prob>    P(bit corruption) in channel      (default 0.20)\n"
	    "  -s <seed>    RNG seed                          (default 1)\n"
	    "  -h           show this help\n"
	    "\n"
	    "Exit codes: 0 = all messages delivered, 1 = bad args, 2 = gave "
	    "up\n",
	    prog, SEQ_MOD);
}

int main(int argc, char** argv) {
	int total = 8, window = 4, timeout_tks = 3, max_rounds = 200, seed = 1, debug = 0;
	double drop = 0.20, corrupt = 0.20;

	int opt;
	while ((opt = getopt(argc, argv, "n:w:t:m:p:c:s:dh")) != -1) {
		switch (opt) {
			case 'n':
				total = atoi(optarg);
				break;
			case 'w':
				window = atoi(optarg);
				break;
			case 't':
				timeout_tks = atoi(optarg);
				break;
			case 'm':
				max_rounds = atoi(optarg);
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
			case 'd':
				debug = 1;
				break;
			case 'h':
				usage(argv[0]);
				return 0;
			default:
				usage(argv[0]);
				return 1;
		}
	}

	if (total < 1 || total > MAX_MSGS) {
		fprintf(stderr, "Invalid message count (1..%d).\n", MAX_MSGS);
		return 1;
	}

	if (window < 1 || window >= SEQ_MOD) {
		fprintf(stderr,
			"Window size must be in [1, %d] "
			"(strictly less than the %d-value seq space).\n",
			SEQ_MOD - 1, SEQ_MOD);
		return 1;
	}

	if (timeout_tks < 1 || max_rounds < 1 || drop < 0 || drop > 1 ||
	    corrupt < 0 || corrupt > 1) {
		fprintf(stderr, "Invalid parameters.\n");
		return 1;
	}

	srand((unsigned)seed);

	char msgs[MAX_MSGS][MAX_DATA];
	for (int i = 0; i < total; i++)
		snprintf(msgs[i], MAX_DATA, "msg-%d", i);

	ChannelParams cp = {drop, corrupt};

	int base, next_idx, expected_seq, delivered, idle_rounds, need_retx;
	base = next_idx = expected_seq = delivered = idle_rounds = need_retx = 0;

	int total_tx, retx_cnt;
	total_tx = retx_cnt = 0;

	printf("=============================================\n");
	printf("      GO-BACK-N SLIDING WINDOW PROTOCOL\n");
	printf("=============================================\n");
	printf("  Messages     : %d\n", total);
	printf("  Window size  : %d\n", window);
	printf("  Seq space    : 0..%d (mod %d)\n", SEQ_MOD - 1, SEQ_MOD);
	printf("  Timeout      : %d round(s)\n", timeout_tks);
	printf("  P(drop)      : %.2f\n", drop);
	printf("  P(corrupt)   : %.2f\n", corrupt);
	printf("  Seed         : %d\n", seed);
	printf("=============================================\n");

	int round = 0;
	for (round = 1; round <= max_rounds; ++round) {
		if (base >= total) break;

		printf("\n=== ROUND %d ===\n", round);
		printf(
		    "[state]  base=%d  next=%d  window=[%d..%d]  "
		    "expected_seq=%d  delivered=%d/%d\n",
		    base, next_idx, base, base + window - 1, expected_seq,
		    delivered, total);

		int to_send[MAX_BURSTS];
		int send_count = 0;

		if (need_retx) {
			for (int i = base; i < next_idx; i++)
				to_send[send_count++] = i;
			printf(
			    "[sender] TIMEOUT -> retransmitting msgs [%d..%d] "
			    "(%d frames)\n",
			    base, next_idx - 1, send_count);
			retx_cnt += send_count;
			need_retx = 0;
			idle_rounds = 0;
		} else {
			while (next_idx < total && next_idx < base + window) {
				to_send[send_count++] = next_idx;
				++next_idx;
			}
			if (send_count > 0)
				printf(
				    "[sender] sending new frames: msgs "
				    "[%d..%d]\n",
				    to_send[0], to_send[send_count - 1]);
			else if (next_idx < total)
				printf(
				    "[sender] window full (%d frames in "
				    "flight) - awaiting ACKs\n", next_idx -
				    base);
		}

		int max_ack_idx = base - 1;

		for (int i = 0; i < send_count; i++) {
			int msg_idx = to_send[i];
			int seq = msg_idx % SEQ_MOD;

			printf("  --> DATA seq=%d (msg %d) [%s]\n", seq, msg_idx,
			       msgs[msg_idx]);

			Frame f;
			gbn_frame_init_data(&f, seq, msgs[msg_idx],
					    strlen(msgs[msg_idx]));
			if (debug) gbn_frame_print(&f, "init ");

			++total_tx;

			if (!channel_transmit(&cp, &f, "frame")) continue;

			if (debug) gbn_frame_print(&f, "after");

			int valid = gbn_frame_is_valid(&f);
			if (debug) printf("   [debug] stored=%04X  recomputed=%04X  valid=%d\n", f.checksum, gbn_frame_recompute_csum(&f), valid);

			int in_order = valid && (f.seq == expected_seq);

			int ack_val;
			if (in_order) {
				printf(
				    "    [receiver] in-order, deliver \"%s\", "
				    "expected_seq %d -> %d\n",
				    msgs[delivered], expected_seq,
				    gbn_next_seq(expected_seq));
				ack_val = expected_seq;
				++delivered;
				expected_seq = gbn_next_seq(expected_seq);
			} else {
				ack_val = gbn_prev_seq(expected_seq);
				printf(
				    "    [receiver] %s -> discard, send dup "
				    "ACK seq=%d\n",
				    valid ? "out-of-order" : "corrupted",
				    ack_val);
			}

			Frame ack;
			gbn_frame_init_ack(&ack, ack_val);
			printf("  <-- ACK seq=%d%s\n", ack_val,
			       in_order ? "" : " (dup)");

			if (!channel_transmit(&cp, &ack, "ACK")) continue;
			if (!gbn_frame_is_valid(&ack)) {
				printf(
				    "    [sender] ACK corrupted - ignored\n");
				continue;
			}

			for (int j = base; j < next_idx; j++) {
				if (j % SEQ_MOD == ack.seq) {
					if (j > max_ack_idx) max_ack_idx = j;
					break;
				}
			}
		}

		if (max_ack_idx >= base) {
			printf("[sender] cumulaitve ACK -> base: %d -> %d\n",
			       base, max_ack_idx + 1);
			base = max_ack_idx + 1;
			idle_rounds = 0;
		} else {
			++idle_rounds;
			printf("[sender] no progress (idle %d/%d round(s))\n",
			       idle_rounds, timeout_tks);
			if (idle_rounds >= timeout_tks && base < next_idx) {
				printf("[sender] timer expired\n");
				need_retx = 1;
				idle_rounds = 0;
			}
		}
	}

	printf("\n=============== SUMMARY ===============\n");
	printf("  Messages delivered : %d / %d\n", delivered, total);
	printf("  Total frames sent  : %d\n", total_tx);
	printf("  Retransmissions    : %d (%.0f%% of total)\n", retx_cnt,
	       total_tx ? 100.0 * retx_cnt / total_tx : 0.0);
	printf("  Rounds             : %d\n", round - 1);
	printf("  Efficiency         : %.2f (delivered / total TX)\n",
	       total_tx ? (double)delivered / total_tx : 0.0);
	printf("  Result             : %s\n",
	       delivered >= total ? "ALL DELIVERED" : "GAVE UP (max rounds)");

	return delivered >= total ? 0 : 2;
}
