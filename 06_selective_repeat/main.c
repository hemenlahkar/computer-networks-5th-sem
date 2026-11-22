#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "channel.h"
#include "sr.h"

#define MAX_MSGS   64
#define MAX_BURSTS 64

static void usage(const char* prog) {
    fprintf(stderr,
        "Usage: %s [options]\n"
        "\n"
        "Selective Repeat ARQ simulation over a noisy channel.\n"
        "\n"
        "Options:\n"
        "  -n <msgs>    number of messages to send        (default 8)\n"
        "  -w <size>    window size (must be <= %d)       (default 4)\n"
        "  -t <rounds>  timeout in rounds                 (default 3)\n"
        "  -m <rounds>  max simulation rounds             (default 200)\n"
        "  -p <prob>    P(frame drop) in channel          (default 0.20)\n"
        "  -c <prob>    P(bit corruption) in channel      (default 0.20)\n"
        "  -s <seed>    RNG seed                          (default 1)\n"
        "  -d           debug output\n"
        "  -h           show this help\n"
        "\n"
        "Note: SR requires window <= SEQ_MOD/2 = %d to avoid ambiguity\n"
        "between retransmitted frames and new frames.\n"
        "\n"
        "Exit codes: 0 = all delivered, 1 = bad args, 2 = gave up\n",
        prog, SEQ_MOD / 2, SEQ_MOD / 2);
}

int main(int argc, char** argv) {
    int total = 8, window = 4, timeout_tks = 3, max_rounds = 200, seed = 1, debug = 0;
    double drop = 0.20, corrupt = 0.20;

    int opt;
    while ((opt = getopt(argc, argv, "n:w:t:m:p:c:s:dh")) != -1) {
        switch (opt) {
            case 'n': total        = atoi(optarg); break;
            case 'w': window       = atoi(optarg); break;
            case 't': timeout_tks  = atoi(optarg); break;
            case 'm': max_rounds   = atoi(optarg); break;
            case 'p': drop         = atof(optarg); break;
            case 'c': corrupt      = atof(optarg); break;
            case 's': seed         = atoi(optarg); break;
            case 'd': debug        = 1;            break;
            case 'h': usage(argv[0]); return 0;
            default:  usage(argv[0]); return 1;
        }
    }

    if (total < 1 || total > MAX_MSGS) {
        fprintf(stderr, "Invalid message count (1..%d).\n", MAX_MSGS);
        return 1;
    }
    if (window < 1 || window > SEQ_MOD / 2) {
        fprintf(stderr, "Window must be in [1, %d] (half of the %d-seq space).\n",
                SEQ_MOD / 2, SEQ_MOD);
        return 1;
    }
    if (timeout_tks < 1 || max_rounds < 1 ||
        drop < 0 || drop > 1 || corrupt < 0 || corrupt > 1) {
        fprintf(stderr, "Invalid parameters.\n");
        return 1;
    }

    srand((unsigned)seed);

    char msgs[MAX_MSGS][MAX_DATA];
    for (int i = 0; i < total; i++)
        snprintf(msgs[i], MAX_DATA, "msg-%d", i);

    ChannelParams cp = { drop, corrupt };

    int base     = 0;
    int next_idx = 0;
    int acked[MAX_MSGS] = {0};
    int timer[MAX_MSGS];
    for (int i = 0; i < MAX_MSGS; i++) timer[i] = -1;

    int rcv_base = 0;
    int received[MAX_MSGS] = {0};

    int total_tx = 0, retx_cnt = 0;
    int round = 0;

    printf("=============================================\n");
    printf("       SELECTIVE REPEAT SLIDING WINDOW\n");
    printf("=============================================\n");
    printf("  Messages     : %d\n", total);
    printf("  Window size  : %d\n", window);
    printf("  Seq space    : 0..%d (mod %d)\n", SEQ_MOD - 1, SEQ_MOD);
    printf("  Timeout      : %d round(s)\n", timeout_tks);
    printf("  P(drop)      : %.2f\n", drop);
    printf("  P(corrupt)   : %.2f\n", corrupt);
    printf("  Seed         : %d\n", seed);
    printf("=============================================\n");

    for (round = 1; round <= max_rounds; ++round) {
        if (rcv_base >= total) break;

        printf("\n=== ROUND %d ===\n", round);
        printf("[state]  base=%d  next=%d  window=[%d..%d]  rcv_base=%d  delivered=%d/%d\n",
               base, next_idx, base, base + window - 1,
               rcv_base, rcv_base, total);

        int expired[MAX_MSGS] = {0};
        for (int i = base; i < next_idx; i++) {
            if (acked[i]) continue;
            if (timer[i] > 0) {
                timer[i]--;
                if (timer[i] == 0) expired[i] = 1;
            }
        }

        int to_send[MAX_BURSTS];
        int is_retx[MAX_BURSTS];
        int send_count = 0, n_retx = 0, n_new = 0;

        for (int i = base; i < next_idx && send_count < MAX_BURSTS; i++) {
            if (acked[i] || !expired[i]) continue;
            to_send[send_count] = i;
            is_retx[send_count] = 1;
            ++send_count; ++n_retx;
        }
        while (next_idx < total && next_idx < base + window &&
               send_count < MAX_BURSTS) {
            to_send[send_count] = next_idx;
            is_retx[send_count] = 0;
            ++send_count; ++next_idx; ++n_new;
        }

        if (send_count == 0)
            printf("[sender] nothing new to send (window full / awaiting ACKs)\n");
        else if (n_retx && n_new)
            printf("[sender] %d retransmission(s) + %d new frame(s)\n", n_retx, n_new);
        else if (n_retx)
            printf("[sender] TIMEOUT -> retransmitting %d frame(s)\n", n_retx);
        else
            printf("[sender] sending %d new frame(s)\n", n_new);

        for (int i = 0; i < send_count; i++) {
            int msg_idx = to_send[i];
            int seq     = msg_idx % SEQ_MOD;
            int is_r    = is_retx[i];

            timer[msg_idx] = timeout_tks;
            printf("  --> DATA seq=%d (msg %d)%s [%s]\n",
                   seq, msg_idx, is_r ? " [RETX]" : "", msgs[msg_idx]);

            Frame f;
            sr_frame_init_data(&f, seq, msgs[msg_idx], strlen(msgs[msg_idx]));
            if (debug) sr_frame_print(&f, "init");

            ++total_tx;
            if (is_r) ++retx_cnt;

            if (!channel_transmit(&cp, &f, "DATA")) continue;

            if (debug) sr_frame_print(&f, "after");
            int valid = sr_frame_is_valid(&f);
            if (debug)
                printf("   [debug] stored=%04X  recomputed=%04X  valid=%d\n",
                       f.checksum, sr_frame_recompute_csum(&f), valid);

            if (!valid) {
                printf("    [receiver] corrupted DATA -> discard (no ACK)\n");
                continue;
            }

            int target = -1;
            for (int k = 0; k < window; k++) {
                int cand = rcv_base + k;
                if (cand < total && (cand % SEQ_MOD) == f.seq) {
                    target = cand;
                    break;
                }
            }

            if (target != -1) {
                if (!received[target]) {
                    received[target] = 1;
                    int old_rcv = rcv_base;
                    while (rcv_base < total && received[rcv_base]) ++rcv_base;
                    if (target == old_rcv)
                        printf("    [receiver] DATA seq=%d (msg %d) in-order -> DELIVER%s"
                               " (rcv_base now %d)\n",
                               f.seq, target,
                               (rcv_base - old_rcv > 1) ? " (+buffered)" : "",
                               rcv_base);
                    else
                        printf("    [receiver] DATA seq=%d (msg %d) out-of-order -> BUFFER (rcv_base stays %d)\n",
                               f.seq, target, rcv_base);
                } else {
                    printf("    [receiver] DATA seq=%d (msg %d) duplicate -> ACK again\n",
                           f.seq, target);
                }
            } else {
                printf("    [receiver] DATA seq=%d outside receive window -> ACK anyway\n",
                       f.seq);
            }

            Frame ack;
            sr_frame_init_ack(&ack, f.seq);
            printf("  <-- ACK seq=%d\n", f.seq);

            if (!channel_transmit(&cp, &ack, "ACK")) continue;
            if (!sr_frame_is_valid(&ack)) {
                printf("    [sender] ACK corrupted -> ignored\n");
                continue;
            }

            int match = -1;
            for (int j = base; j < next_idx; j++) {
                if (!acked[j] && (j % SEQ_MOD) == ack.seq) { match = j; break; }
            }
            if (match != -1) {
                acked[match] = 1;
                timer[match] = -1;
                printf("    [sender] ACK seq=%d -> mark msg %d acked\n",
                       ack.seq, match);
                int old_base = base;
                while (base < next_idx && acked[base]) ++base;
                if (base > old_base)
                    printf("    [sender] slide window -> base now %d\n", base);
            } else {
                printf("    [sender] ACK seq=%d -> stale/duplicate, ignored\n", ack.seq);
            }
        }
    }

    printf("\n=============== SUMMARY ===============\n");
    printf("  Messages delivered : %d / %d  (rcv_base)\n", rcv_base, total);
    printf("  Sender base        : %d (contiguous ACKed prefix)\n", base);
    printf("  Total frames sent  : %d\n", total_tx);
    printf("  Retransmissions    : %d (%.0f%% of total)\n",
           retx_cnt, total_tx ? 100.0 * retx_cnt / total_tx : 0.0);
    printf("  Rounds             : %d\n", round - 1);
    printf("  Efficiency         : %.2f (delivered / total TX)\n",
           total_tx ? (double)rcv_base / total_tx : 0.0);
    printf("  Result             : %s\n",
           rcv_base >= total ? "ALL DELIVERED" : "GAVE UP (max rounds)");

    return rcv_base >= total ? 0 : 2;
}
