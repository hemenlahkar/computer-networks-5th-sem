#include "sr.h"

#include <stdio.h>
#include <string.h>

uint16_t sr_checksum(const void* buf, size_t len) {
    const uint8_t* p = buf;
    uint32_t sum = 0;
    size_t i = 0;
    for (; i + 1 < len; i += 2) sum += ((uint32_t)p[i] << 8) | p[i + 1];
    if (i < len) sum += ((uint32_t)p[i] << 8);
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)~sum;
}

static size_t frame_body_bytes(const Frame *f, uint8_t *buf, size_t cap) {
    if (f->len > MAX_DATA) return 0;
    size_t need = 4 + f->len;
    if (cap < need) return 0;

    size_t n = 0;
    buf[n++] = (uint8_t)((unsigned)f->kind & 0xFFu);
    buf[n++] = (uint8_t)((unsigned)f->seq  & 0xFFu);
    buf[n++] = (uint8_t)(((unsigned)f->len >> 8) & 0xFFu);
    buf[n++] = (uint8_t)((unsigned)f->len       & 0xFFu);
    if (f->len) {
        memcpy(buf + n, f->data, f->len);
        n += f->len;
    }
    return n;
}

static uint16_t compute_frame_csum(const Frame* f) {
    uint8_t buf[MAX_DATA + 8];
    size_t n = frame_body_bytes(f, buf, sizeof buf);
    if (n == 0) return 0;
    return sr_checksum(buf, n);
}

void sr_frame_init_data(Frame* f, int seq, const char* payload, size_t len) {
    memset(f, 0, sizeof *f);
    if (len > MAX_DATA) len = MAX_DATA;
    f->kind = FRAME_DATA;
    f->seq  = seq;
    f->len  = len;
    if (len) memcpy(f->data, payload, len);
    f->checksum = compute_frame_csum(f);
}

void sr_frame_init_ack(Frame* f, int seq) {
    memset(f, 0, sizeof *f);
    f->kind = FRAME_ACK;
    f->seq  = seq;
    f->len  = 0;
    f->checksum = compute_frame_csum(f);
}

int sr_frame_is_valid(const Frame* f) {
    return f->checksum == compute_frame_csum(f);
}

void sr_frame_print(const Frame* f, const char* who) {
    if (f->kind == FRAME_DATA)
        printf("  [%s] DATA seq=%d len=%zu csum=%04X \"%.*s\"\n",
               who, f->seq, f->len, f->checksum, (int)f->len, f->data);
    else
        printf("  [%s] ACK seq=%d csum=%04X\n", who, f->seq, f->checksum);
}

uint16_t sr_frame_recompute_csum(const Frame *f) {
    return compute_frame_csum(f);
}
