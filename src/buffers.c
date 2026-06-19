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
	buffer->text = (char *)(buffer + 1);

	if (file) {
		fseek(file, 0, SEEK_END);
		size_t length = ftell(file);
		fseek(file, 0, SEEK_SET);

		fread(buffer->text, 1, length, file);
		buffer->text_len = length;
	} else {
		memset(buffer->text, 0, MAX_BUFFER_TEXT_LEN);
		buffer->text_len = 0;
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
