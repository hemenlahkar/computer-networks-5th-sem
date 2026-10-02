#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>

#include "framing.h"

#define MAXBUF 8192

static void usage(const char *prog)
{
    fprintf(stderr,
            "Usage:\n"
            "  %s -m bit  -d <binary-string>\n"
            "  %s -m char -d <text>\n"
            "  %s -m char -x <hex-bytes>\n"
            "  %s -D\t\t(run built-in demos)\n"
            "\n"
            "Options:\n"
            "  -m <mode>    'bit' or 'char' framing\n"
            "  -d <data>    payload: binary string for bit mode, text for char mode\n"
            "  -x <hex>     payload as hex bytes for char mode (e.g. 7E7D48)\n"
            "  -D           run both demonstrative examples\n"
            "  -h           this help\n"
            "\n"
            "Exit codes: 0 ok, 1 bad arguments, 2 framing error\n",
            prog, prog, prog, prog);
}

static int parse_hex(const char *s, unsigned char *out, size_t out_size, size_t *out_len)
{
    size_t n = strlen(s);
    if (n % 2 != 0)
        return -1;
    if (n / 2 > out_size)
        return -1;
    for (size_t i = 0; i < n; i += 2)
    {
        int hi, lo;
        char a = s[i], b = s[i + 1];
        if (!isxdigit((unsigned char)a) || !isxdigit((unsigned char)b))
            return -1;
        hi = isdigit((unsigned char)a) ? a - '0' : (tolower(a) - 'a' + 10);
        lo = isdigit((unsigned char)b) ? b - '0' : (tolower(b) - 'a' + 10);
        out[i / 2] = (unsigned char)((hi << 4) | lo);
    }
    *out_len = n / 2;
    return 0;
}

static int text_to_bits(const char *text, char *bits, size_t bits_size)
{
    size_t j = 0;
    for (size_t i = 0; text[i]; i++)
    {
        unsigned char c = (unsigned char)text[i];
        for (int b = 7; b >= 0; --b)
        {
            if (j + 2 >= bits_size)
                return -1;
            bits[j++] = ((c >> b) & 1) ? '1' : '0';
        }
    }
    bits[j] = '\0';
    return 0;
}

static int run_bit_mode(const char *data)
{
    char frame[MAXBUF];
    char recovered[MAXBUF];

    printf("======== BIT STUFFING (HDLC/PPP) ========\n");
    printf("Flag byte             : 01111110 (0x7E)\n");
    printf("Stuffing rule         : insert '0' after every 5 consecutive 1s\n\n");
    print_bits_labeled("Original data", data);

    if (bit_stuff(data, frame, sizeof(frame)) != FR_OK)
    {
        fprintf(stderr, "Error: bit_stuff failed (buffer too small or bad data).\n");
        return 2;
    }
    print_bits_labeled("Stuffed frame", frame);

    printf("%-22s: ", "Stuffing positions");
    {
        const char *p = frame + 8;
        const char *end = frame + strlen(frame) - 8;
        int ones = 0;
        size_t idx = 0;
        for (; p < end; ++p, ++idx)
        {
            if (*p == '1')
            {
                ++ones;
                if (ones == 5)
                {
                    printf(" ^ (after bit %zu of payload)", idx);
                    ++p;
                    ++idx;
                    ones = 0;
                }
            }
            else
            {
                ones = 0;
            }
        }
        putchar('\n');
    }

    if (bit_unstuff(frame, recovered, sizeof(recovered)) != FR_OK)
    {
        fprintf(stderr, "Error: bit_unstuff failed.\n");
        return 2;
    }
    print_bits_labeled("Recovered data", recovered);

    if (strcmp(data, recovered) != 0)
    {
        printf("Result    : MISMATCH\n");
        return 2;
    }
    printf("Result    : Round-trip OK\n");
    return 0;
}

static int run_char_mode(const unsigned char *data, size_t len)
{
    unsigned char frame[MAXBUF];
    unsigned char recovered[MAXBUF];
    size_t frame_len = 0, rec_len = 0;

    printf("======== CHARACTER STUFFING (PPP) =======\n");
    printf("FLAG = 0x7E, ESC = 0x7D\n");
    printf("Stuffing rule         : 0x7E -> 7D 5E, 0x7D -> 7D 5D\n\n");

    print_hex_labeled("Original payload", data, len);

    if (char_stuff(data, len, frame, sizeof(frame), &frame_len) != FR_OK)
    {
        fprintf(stderr, "Error: char_stuff failed.\n");
        return 2;
    }
    print_hex_labeled("Stuffed frame", frame, frame_len);

    if (char_unstuff(frame, frame_len, recovered, sizeof(recovered), &rec_len) != FR_OK)
    {
        fprintf(stderr, "Error: char_unstuff failed.\n");
        return 2;
    }
    print_hex_labeled("Recovered payload", recovered, rec_len);

    if (rec_len != len || memcmp(data, recovered, len) != 0)
    {
        printf("Result    : MISTMATCH\n");
        return 2;
    }
    printf("Result    : Round-trip OK\n");
    return 0;
}

static int run_demos(void)
{
    if (run_bit_mode("111110111111011111101") != 0)
        return 2;
    putchar('\n');

    const unsigned char payload[] = {'H', 'e', 'l', 'l', 'o', 0x7E, 'W', 0x7D, '!'};
    if (run_char_mode(payload, sizeof(payload)) != 0)
        return 2;

    return 0;
}

int main(int argc, char **argv)
{
    const char *mode = NULL;
    const char *data = NULL;
    const char *hex = NULL;
    int demo = 0;

    int opt;
    while ((opt = getopt(argc, argv, "m:d:x:Dh")) != -1)
    {
        switch (opt)
        {
        case 'm':
            mode = optarg;
            break;
        case 'd':
            data = optarg;
            break;
        case 'x':
            hex = optarg;
            break;
        case 'D':
            demo = 1;
            break;
        case 'h':
            usage(argv[0]);
            return 0;
        case '?':
        default:
            usage(argv[0]);
            return 1;
        }
    }

    if (optind < argc) {
        fprintf(stderr, "Unexpected argument: %s\n", argv[optind]);
        usage(argv[0]);
        return 1;
    }

    if (demo) return run_demos();

    if (!mode) {
        fprintf(stderr, "Error: -m <bit|char> is required (or use -D for demos).\n");
        usage(argv[0]);
        return 1;
    }

    if (strcmp(mode, "bit") == 0) {
        if (!data) {
            fprintf(stderr, "bit mode requires -d <binary string>\n");
            return 1;
        }

        int is_binary = (*data != '\0');
        for(const char *p = data; *p; ++p) {
            if (*p != '0' && *p != '1') {is_binary = 0; break;}
        }
        if (!is_binary) {
            char bits[MAXBUF];
            if (text_to_bits(data, bits, sizeof(bits)) != 0) {
                fprintf(stderr, "Error: input too long to convert to bits.\n");
                return 1;
            }
            printf("(Note: '%s' is not binary; converted to bits.)\n\n", data);
            return run_bit_mode(bits);
        }
        return run_bit_mode(data);
    } else if (strcmp(mode, "char") == 0) {
        unsigned char buf[MAXBUF];
        size_t buf_len = 0;

        if (hex) {
            if (parse_hex(hex, buf, sizeof(buf), &buf_len) != 0) {
                fprintf(stderr, "Bad hex input '%s'.\n", hex);
                return 1;
            }
        } else if (data) {
            buf_len = strlen(data);
            if (buf_len > sizeof(buf)) {
                fprintf(stderr, "Input too long.\n");
                return 1;
            }
            memcpy(buf, data, buf_len);
        } else {
            fprintf(stderr, "char mode requires -d <text> or -x <hex>\n");
            return 1;
        }
        return run_char_mode(buf, buf_len);
    }

    fprintf(stderr, "Unknown mode '%s' (expected 'bit' or 'char')\n", mode);
    return 1;
}