#ifndef INTERNALS_H
#define INTERNALS_H

#include <mr_utils.h>
#include <stdint.h>
#include <stdio.h>

// NOTE: uint8_max because typedef uint8_t BufferIndex;
#define MAX_BUFFERS UINT8_MAX
#define MIN_BUFFER_TEXT_LENGTH UINT8_MAX

typedef uint8_t BufferIndex;

// TODO: how to handle large files
typedef struct {
	char *text; // TODO: const?
	size_t text_len;

	FILE *write_to;

	size_t cursor_x;
	size_t cursor_y;
} Buffer;

extern BufferIndex buffer_count;
extern Buffer *buffers[MAX_BUFFERS];

Buffer *buffer_create(FILE *file);
void buffer_destory(Buffer *buffer);

#define BUFFERS_GET_BUFFER_BY_IDX(idx) (buffers[(idx)])

#endif
