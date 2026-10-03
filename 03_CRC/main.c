#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <getopt.h>

#include "crc.h"
#include "channel.h"

#define MAX_FRAME 8192

static void usage(const char *prog)
{
    fprintf(stderr,
            "Usage: %s -d <data> -g <generator> [options]\n"
            "\n"
            "Required:\n"
            "\t-d <bits>\tData bits (binary string, e.g. 1101011011)\n"
            "\t-g <bits>\tGenerator polynomial (binary, first & last bit = 1)\n"
            "\n"
            "Optional:\n"
            "\t-p <pos>\tFlip the bit at position pos (0 = leftmost of frame)\n"
            "\t-r <n>\tInject n random bit errors\n"
            "\t-s <seed> Random seed (default: time-based)\n"
            "\t-v\t\tVerbose: print modulo-2 divison steps\n"
            "\t-h\t\tShow this help\n"
            "\n"
            "Exit codes: 0 = no error detected at receiver\n"
            "            1 = bad arguments\n"
            "            2 = error detected (frame discarded)\n"
            "\n"
            "Examples:\n"
            "    %s -d 1101011011 -g 10011 -p 5 -v\n"
            "    %s -d 1101011011 -g 10011 -r 3 -s 42\n",
            prog, prog, prog);
}

int main(int argc, char **argv)
{
    const char *data = NULL;
    const char *gen = NULL;
    int flipPos, randomErrs, seed, verbose;
    flipPos = seed = -1;
    randomErrs = verbose = 0;

    int opt;
    while ((opt = getopt(argc, argv, "d:g:p:r:s:vh")) != -1)
    {
        switch (opt)
        {
        case 'd':
            data = optarg;
            break;
        case 'g':
            gen = optarg;
            break;
        case 'p':
            flipPos = atoi(optarg);
            break;
        case 'r':
            randomErrs = atoi(optarg);
            break;
        case 's':
            seed = atoi(optarg);
            break;
        case 'v':
            verbose = 1;
            break;
        case 'h':
            usage(argv[0]);
            return 0;
        default:
            usage(argv[0]);
            return 1;
        }
    }

    if (!data || !gen) {usage(argv[0]); return 1;}

    if (!crc_is_binary(data)) {
        fprintf(stderr, "Error: data must be a binary string.\n");
        return 1;
    }

    if (!crc_is_valid_generator(gen)) {
        fprintf(stderr, "Error: generator must be binary, length >= 2, "
        "with first and last bits = 1.\n");
        return 1;
    }

    size_t dataLen = strlen(data);
    size_t genLen = strlen(gen);
    if (dataLen + 2 * genLen + 2 > MAX_FRAME) {
        fprintf(stderr, "Error: input too long.\n");
        return 1;
    }

    char crc[MAX_FRAME];
    char frame[MAX_FRAME];
    char received[MAX_FRAME];

    FILE *trace = verbose ? stdout : NULL;

    printf("======== SENDER ========\n");
    printf("Data    : %s\n", data);
    printf("Generator    : %s\n", gen);

    if (crc_generate(data, gen, crc, sizeof(crc), trace) != CRC_OK) {
        fprintf(stderr, "Error: CRC generation failed.\n");
        return 1;
    }
    printf("CRC remainder    : %s\n", crc);

    snprintf(frame, sizeof(frame), "%s%s", data, crc);
    printf("Transmitte frame   : %s\n", frame);

    snprintf(received, sizeof(received), "%s", frame);

    printf("\n======== CHANNEL ========\n");
    int injected = 0;

    if (flipPos >= 0) {
        if ((size_t)flipPos >= strlen(received)) {
            fprintf(stderr, "Error: position %d out of range [0, %zu].\n", flipPos, strlen(received) - 1);
            return 1;
        }
        channel_flip_bit(received, (size_t)flipPos);
        printf("Flipped bit at position %d\n", flipPos);
        injected++;
    }

    if (randomErrs > 0) {
        if (seed < 0) seed = (int)time(NULL);
        srand((unsigned)seed);
        printf("Injecting up to %d random error(s) (seed=%d)\n", randomErrs, seed);
        injected += channel_inject_random_errors(received, randomErrs);
    }

    if (injected == 0) {
        printf("Noiseless channel: frame passed through unchanged.\n");
    }
    printf("Received frame    : %s\n", received);

    printf("\n======== RECEIVER ========\n");
    int ok = crc_verify(received, gen, trace);

    if (ok) {
        size_t dataOutLen = strlen(received) - (genLen - 1);
        printf("Result    : No error detected\n");
        printf("Extracted data    : %.*s\n", (int)dataOutLen, received);
        return 0;
    } else {
        printf("Result    : ERROR DETECTED (frame discarded)\n");
        return 2;
    }
}