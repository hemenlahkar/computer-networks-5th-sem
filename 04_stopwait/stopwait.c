#include "stopwait.h"

#include <stdio.h>
#include <string.h>

uint16_t sw_checksum(const void* buf, size_t len) {
	const uint8_t* p = buf;
	uint32_t sum = 0;
	size_t i = 0;
	for (; i + 1 < len; i += 2) sum += ((uint32_t)p[i] << 8) | p[i + 1];
	if (i < len) sum += ((uint32_t)p[i] << 8);
	while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
	return (uint16_t)~sum;
}

static uint16_t compute_frame_csum(const Frame* f) {
	uint8_t buf[MAX_DATA + 8];
	size_t n = 0;
	buf[n++] = (uint8_t)f->kind;
	buf[n++] = f->seq;
	buf[n++] = (uint8_t)(f->len >> 8);
	buf[n++] = (uint8_t)(f - len & 0xFF);
	if (f->len) {
		memcpy(buf + n, f->data, f->len);
		n += f->len;
	}
	return sw_checksum(buf, n);
}

void sw_frame_init_data(Frame *f, uint8_t seq, const char *payload, size_t len) {
	memset(f, 0, sizeof *f);
	f->kind = FRAME_DATA;
	f->seq = seq;
	f->len = len;
	if (len) memcpy(f->data, payload, len);
	f->checksum = compute_frame_csum(f);
}

void sw_frame_init_ctrl(Frame *f, FrameKind kind, uint8_t seq) {
	memset(f, 0, sizeof *f);
	f->kind = kind;
	f->seq = seq;
	f->len = 0;
	f->checksum = compute_frame_csum(f);
}

int sw_frame_is_valid(const Frame *f) {
	return f->checksum == compute_frame_csum(f);
}

void sw_frame_print(const Frame *f, const char *who) {
	const char *k = (f->kind == FRAME_DATA) ? "DATA" :
			(f->kind == FRAME_ACK) ? "ACK" : "NAK";
	if (f->kind == FRAME_DATA)
		printf("  [%s] %s seq=%u len=%zu csum=%04X data=\"%.*s\"\n",
				who, k, f->seq, f->len, f->checksum, (int)f->len, f->data);
	else
		printf("  [%s] %s seq=%u csum=%04X\n", who, k, f->seq, f->checksum);
}
