#include "channel.h"
#include <stdlib.h>
#include <string.h>

void channel_flip_bit(char *frame, size_t pos) {
    if (!frame) return;
    if (pos >= strlen(frame)) return;
    frame[pos] = (frame[pos] == '0') ? '1' : '0';
}

int channel_inject_random_errors(char *frame, int n) {
    if (!frame || n <= 0) return 0;

    size_t len = strlen(frame);
    if ((size_t)n > len) n = (int)len;

    size_t *pos = malloc(len * sizeof(size_t));
    if(!pos) return 0;
    for (size_t i = 0; i < len; i++) pos[i] = i;

    for (int i = 0; i < n; i++)
    {
        size_t remaining = len - (size_t)i;
        size_t j = (size_t)i + (size_t)(rand() % (int)remaining);
        size_t tmp = pos[i];
        pos[i] = pos[j];
        pos[j] = tmp;
        channel_flip_bit(frame, pos[i]);
    }

    free(pos);
    return n;
}