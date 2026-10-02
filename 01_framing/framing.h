#ifndef _FRAMING_H_
#define _FRAMING_H_

#include <stddef.h>

#define FR_OK 0
#define FR_ERR -1

int bit_stuff(const char *data, char *out, size_t out_size);

int bit_unstuff(const char *frame, char *out, size_t out_size);

int char_stuff(const unsigned char *data, size_t len, unsigned char *out, size_t out_size, size_t *out_len);

int char_unstuff(const unsigned char *frame, size_t len, unsigned char *out, size_t out_size, size_t *out_len);

void print_bits_labeled(const char *label, const char *bits);
void print_hex_labeled(const char *label, const unsigned char *buf, size_t len);

#endif