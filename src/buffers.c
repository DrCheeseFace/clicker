#include "./internal.h"
#include <mr_utils.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

Buffer *buffers[MAX_BUFFERS] = { NULL };
BufferIndex buffer_count = 0;

Buffer *
buffer_create(FILE *file)
{
	Buffer *const buffer =
		mmap(NULL, BUFFER_ALLOC_SIZE, PROT_READ | PROT_WRITE,
		     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (buffer == MAP_FAILED) {
		return NULL;
	}

	memset(buffer, 0, sizeof(*buffer));

	buffer->write_to = file;

	if (file) {
		fseek(file, 0, SEEK_END);
		long length = ftell(file);
		fseek(file, 0, SEEK_SET);

		if (length < 0) {
			length = 0;
		}

		if ((size_t)length > MAX_BUFFER_TEXT_LEN) {
			length = MAX_BUFFER_TEXT_LEN;
		}

		buffer->gap_start = 0;
		buffer->gap_end = MAX_BUFFER_TEXT_LEN - length;

		fread(buffer->text + buffer->gap_end, 1, length, file);
	} else {
		memset(buffer->text, 0, MAX_BUFFER_TEXT_LEN);
		buffer->gap_start = 0;
		buffer->gap_end = MAX_BUFFER_TEXT_LEN;
	}

	buffers[buffer_count] = buffer;
	buffer_count++;

	return buffer;
}

void
buffer_destroy(Buffer *buffer)
{
	for (BufferIndex i = 0; i < buffer_count; ++i) {
		if (buffers[i] == buffer) {
			buffers[i] = buffers[buffer_count - 1];
			buffers[buffer_count - 1] = NULL;
			buffer_count--;
			break;
		}
	}

	munmap(buffer, BUFFER_ALLOC_SIZE);
}

void
buffer_move_gap(Buffer *buffer, size_t gap_start)
{
	if (buffer->gap_start == gap_start)
		return;

	size_t bytes_to_move;

	if (gap_start < buffer->gap_start) {
		bytes_to_move = buffer->gap_start - gap_start;
		memmove(buffer->text + buffer->gap_end - bytes_to_move,
			buffer->text + buffer->gap_start - bytes_to_move,
			bytes_to_move);

		buffer->gap_end = buffer->gap_end - bytes_to_move;
	} else {
		bytes_to_move = gap_start - buffer->gap_start;
		memmove(buffer->text + buffer->gap_start,
			buffer->text + buffer->gap_end, bytes_to_move);
		buffer->gap_end = buffer->gap_end + bytes_to_move;
	}

	buffer->gap_start = gap_start;
	return;
}
