#ifndef _CRC_H_
#define _CRC_H_

#include <stddef.h>
#include <stdio.h>

#define CRC_OK 0
#define CRC_ERR -1

int crc_is_binary(const char *s);

int crc_is_valid_generator(const char *gen);

int crc_remainder(const char *dividend, const char *divisor, char *out, size_t out_size, FILE *trace);

int crc_generate(const char *data, const char *gen, char *crc, size_t crc_size, FILE *trace);

int crc_verify(const char *frame, const char *gen, FILE *trace);

#endif