#ifndef __SR_H__
#define __SR_H__

#include <stddef.h>
#include <stdint.h>

#define MAX_DATA 256
#define SEQ_MOD 8

typedef enum {
    FRAME_DATA,
    FRAME_ACK
} FrameKind;

typedef struct {
    FrameKind kind;
    int seq;
    uint16_t checksum;
    size_t len;
    char data[MAX_DATA];
} Frame;

uint16_t sr_checksum(const void *buf, size_t len);

void sr_frame_init_data(Frame *f, int seq, const char *payload, size_t len);
void sr_frame_init_ack(Frame *f, int seq);

int  sr_frame_is_valid(const Frame *f);
void sr_frame_print(const Frame *f, const char *who);
uint16_t sr_frame_recompute_csum(const Frame *f);

static inline int sr_prev_seq(int s) { return (s + SEQ_MOD - 1) % SEQ_MOD; }
static inline int sr_next_seq(int s) { return (s + 1) % SEQ_MOD; }

#endif
