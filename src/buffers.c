#include "buffers.h"
#include <mr_utils.h>
#include <stdlib.h>
#include <string.h>

Buffer *buffers[MAX_BUFFERS] = { NULL };
BufferIndex buffer_count = 0;

// TODO: should this instead take a filepath
Buffer *
buffer_create(FILE *file)
{
	Buffer *const buffer = malloc(sizeof(*buffer));
	if (!buffer) {
		return NULL;
	}
	memset(buffer, 0, sizeof(*buffer));

	buffer->write_to = file;

	if (file) {
		fseek(file, 0, SEEK_END);
		size_t length = ftell(file);
		fseek(file, 0, SEEK_SET);

		buffer->text = malloc(length);
		if (!buffer->text) {
			free(buffer);
			return NULL;
		}

		fread(buffer->text, 1, length, file);

		buffer->text_len = length;
	} else {
		buffer->text = malloc(MIN_BUFFER_TEXT_LENGTH);
		if (!buffer->text) {
			free(buffer);
			return NULL;
		}
		memset(buffer->text, 0, MIN_BUFFER_TEXT_LENGTH);

		buffer->text_len = MIN_BUFFER_TEXT_LENGTH;
	}

	buffers[buffer_count] = buffer;
	buffer_count++;

	return buffer;
}

// TODO: should this close the FILE*
void
buffer_destory(Buffer *buffer)
{
	for (BufferIndex i = 0; i < buffer_count; i++) {
		if (buffers[i] == buffer) {
			buffer_count--;
			buffers[i] = buffers[buffer_count];
			buffers[buffer_count] = NULL;
			break;
		}
	}

	free(buffer->text);
	free(buffer);
}
