#include "framing.h"
#include <stdio.h>
#include <string.h>

#define BIT_FLAG "01111110"
#define FLAG_BYTE 0x7E
#define ESC_BYTE 0x7D

int bit_stuff(const char *data, char *out, size_t out_size) {
    if (!data || !out) return FR_ERR;

    const size_t flaglen = 8;
    size_t j = 0;
    int ones = 0;

    if (out_size <= flaglen) return FR_ERR;
    memcpy(out +j, BIT_FLAG, flaglen);
    j += flaglen;

    for (size_t i = 0; data[i]; i++)
    {
        if (data[i] != '0' && data[i] != '1') return FR_ERR;
        if (j + 2 >= out_size) return FR_ERR;

        out[j++] = data[i];

        if (data[i] == '1') {
            if (++ones == 5) {
                out[j++] = '0';
                ones = 0;
            }
        } else {
            ones = 0;
        }
    }

    if (j + flaglen + 1 > out_size) return FR_ERR;
    memcpy(out + j, BIT_FLAG, flaglen);
    j += flaglen;
    out[j] = '\0';
    return FR_OK;
}

int bit_unstuff(const char *frame, char *out, size_t out_size) {
    if (!frame || !out) return FR_ERR;

    const size_t flaglen = 8;
    size_t framelen = strlen(frame);
    if (framelen < 2 * flaglen) return FR_ERR;

    if (strncmp(frame, BIT_FLAG, flaglen) != 0) return FR_ERR;
    if (strncmp(frame + framelen - flaglen, BIT_FLAG, flaglen) != 0) return FR_ERR;

    const char *p = frame + flaglen;
    const char *end = frame + framelen - flaglen;

    size_t j = 0;
    int ones = 0;

    while (p < end)
    {
        if (j + 1 >= out_size) return FR_ERR;

        char c = *p++;
        out[j++] = c;

        if (c == '1') {
            if (++ones == 5) {
                if (p >= end || *p != '0') return FR_ERR;
                ++p;
                ones = 0;
            }
        } else {
            ones = 0;
        }
    }
    out[j] = '\0';
    return FR_OK;
}

int char_stuff(const unsigned char *data, size_t len, unsigned char *out, size_t out_size, size_t *out_len) {
    if (!data || !out || !out_len) return FR_ERR;

    size_t j = 0;

    if (j + 1 > out_size) return FR_ERR;
    out[j++] = FLAG_BYTE;

    for (size_t i = 0; i < len; i++)
    {
        unsigned char b = data[i];
        if (b == FLAG_BYTE || b == ESC_BYTE) {
            if (j + 2 > out_size) return FR_ERR;
            out[j++] = ESC_BYTE;
            out[j++] = (unsigned char)(b ^ 0x20);
        } else {
            if (j + 1 > out_size) return FR_ERR;
            out[j++] = b;
        }
    }
    
    if (j + 1 > out_size) return FR_ERR;
    out[j++] = FLAG_BYTE;

    *out_len = j;
    return FR_OK;
}

int char_unstuff(const unsigned char *frame, size_t len, unsigned char *out, size_t out_size, size_t *out_len) {
    if (!frame || !out || !out_len) return FR_ERR;
    if (len < 2) return FR_ERR;
    if (frame[0] != FLAG_BYTE || frame[len - 1] != FLAG_BYTE) return FR_ERR;

    size_t j = 0;
    for (size_t i = 1; i < len - 1; i++)
    {
        unsigned char b = frame[i];

        if (b == ESC_BYTE) {
            if (i + 1 >= len - 1) return FR_ERR;
            ++i;
            b = (unsigned char)(frame[i] ^ 0x20);
        }
        if (j + 1 > out_size) return FR_ERR;
        out[j++] = b;
    }
    *out_len = j;
    return FR_OK;    
}

void print_bits_labeled(const char *label, const char *bits) {
    printf("%-22s: %s\n", label, bits);
}

void print_hex_labeled(const char *label, const unsigned char *buf, size_t len) {
    printf("%-22s:", label);
    for(size_t i = 0; i < len; i++) printf(" %02X", buf[i]);
    printf(" (\"");
    for (size_t i = 0; i < len; i++)
    {
        unsigned char c = buf[i];
        putchar((c >= 0x20 && c < 0x7F) ? c : '.');
    }
    printf("\")\n");
}