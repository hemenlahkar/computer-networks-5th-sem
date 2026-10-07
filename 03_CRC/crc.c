#include "crc.h"
#include <string.h>
#include <stdlib.h>

int crc_is_binary(const char *s)
{
    if (!s || !(*s))
        return 0;
    for (const char *p = s; *p; ++p)
        if (*p != '0' && *p != '1')
            return 0;
    return 1;
}

int crc_is_valid_generator(const char *gen)
{
    if (!crc_is_binary(gen))
        return 0;
    size_t n = strlen(gen);
    if (n < 2)
        return 0;
    if (gen[0] != '1' || gen[n - 1] != '1')
        return 0;
    return 1;
}

int crc_remainder(const char *dividend, const char *divisor, char *out, size_t out_size, FILE *trace)
{
    size_t divLen = strlen(divisor);
    size_t msgLen = strlen(dividend);

    if (divLen < 2 || msgLen < divLen)
        return CRC_ERR;
    if (out_size < divLen)
        return CRC_ERR;

    char *buf = malloc(msgLen + 1);
    if (!buf)
        return CRC_ERR;
    memcpy(buf, dividend, msgLen + 1);

    if (trace)
    {
        fprintf(trace, "\n--- Modulo-2 division (divisor = %s) ---\n", divisor);
        fprintf(trace, "%s (dividend)\n", buf);
    }

    for (size_t i = 0; i + divLen <= msgLen; i++)
    {
        if (buf[i] == '1')
        {
            for (size_t j = 0; j < divLen; j++)
            {
                buf[i + j] = (buf[i + j] == divisor[j]) ? '0' : '1';
            }
            if (trace)
            {
                fprintf(trace, "%s (XOR at column %zu)\n", buf, i);
            }
        }
    }

    memcpy(out, buf + (msgLen - divLen + 1), divLen - 1);
    out[divLen - 1] = '\0';

    free(buf);
    return CRC_OK;
}

int crc_generate(const char *data, const char *gen, char *crc, size_t crc_size, FILE *trace)
{
    size_t dataLen = strlen(data);
    size_t genLen = strlen(gen);
    if (genLen < 2 || crc_size < genLen)
        return CRC_ERR;

    size_t paddedLen = dataLen + genLen - 1;
    char *padded = malloc(paddedLen + 1);
    if (!padded)
        return CRC_ERR;

    memcpy(padded, data, dataLen);
    memset(padded + dataLen, '0', genLen - 1);
    padded[paddedLen] = '\0';

    int rc = crc_remainder(padded, gen, crc, crc_size, trace);
    free(padded);
    return rc;
}

int crc_verify(const char *frame, const char *gen, FILE *trace)
{
    size_t genLen = strlen(gen);
    if (genLen < 2)
        return 0;

    char *rem = malloc(genLen);
    if (!rem)
        return 0;

    int rc = crc_remainder(frame, gen, rem, genLen, trace);
    int ok = 0;
    if (rc == CRC_OK)
    {
        ok = 1;
        for (size_t i = 0; i < genLen - 1; i++)
        {
            if (rem[i] != '0')
            {
                ok = 0;
                break;
            }
        }
    }
    free(rem);
    return ok;
}
