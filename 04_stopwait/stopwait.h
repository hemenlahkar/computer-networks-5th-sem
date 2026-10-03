#ifndef __STOPWAIT_H__
#define __STOPWAIT_H__

#include <stddef.h>
#include <stdint.h>

#define MAX_DATA 256

typedef enum {
	FRAME_DATA,
	FRAME_ACK,
	FRAME_NAK
} FrameKind;

typedef struct {
	FrameKind kind;
	uint8_t seq;
	uint16_t checksum;
	size_t len;
	char data[MAX_DATA];
} Frame;

uint16_t sw_checksum(const void *buf, size_t len);

void sw_frame_init_data(Frame *f, uint8_t seq, const char *payload, size_t len);
void sw_frame_init_ctrl(Frame *f, FrameKind kind, uint8_t seq);

int sw_frame_is_valid(const Frame *f);

void sw_frame_print(const Frame *f, const char *who);

#endif
