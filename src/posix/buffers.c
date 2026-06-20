#include "../internal.h"
#include <mr_utils.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

Buffer *buffers[MAX_BUFFERS] = { NULL };
BufferIndex buffer_count = 0;

Buffer *
buffer_create(FILE *file, size_t size)
{
	long page_size = sysconf(_SC_PAGESIZE);
	if (page_size == -1) {
		return NULL;
	}
	size = (size + (size_t)page_size - 1) & ~((size_t)page_size - 1);

	Buffer *const buffer = mmap(NULL, size, PROT_READ | PROT_WRITE,
				    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (buffer == MAP_FAILED) {
		return NULL;
	}

	memset(buffer, 0, sizeof(*buffer));

	buffer->write_to = file;
	buffer->size = size;

	if (file) {
		fseek(file, 0, SEEK_END);
		long length = ftell(file);
		fseek(file, 0, SEEK_SET);

		if (length < 0) {
			length = 0;
		}

		if ((size_t)length > size) {
			length = BUFFER_MAX_TEXT_LENGTH(size);
		}

		buffer->gap_start = 0;
		buffer->gap_end = BUFFER_MAX_TEXT_LENGTH(size) - length;

		fread(buffer->text + buffer->gap_end, 1, length, file);
	} else {
		memset(buffer->text, 0, BUFFER_MAX_TEXT_LENGTH(size));
		buffer->gap_start = 0;
		buffer->gap_end = BUFFER_MAX_TEXT_LENGTH(size);
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

	munmap(buffer, buffer->size);
}

void
buffer_move_gap(Buffer *const buffer, size_t gap_start)
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

void
buffer_insert_char(Buffer **buffer_ptr, char c)
{
	if ((*buffer_ptr)->gap_start == (*buffer_ptr)->gap_end) {
		buffer_expand_gap_by_page(buffer_ptr);
		ASSERT((*buffer_ptr) != NULL);
	}

	*((*buffer_ptr)->text + (*buffer_ptr)->gap_start) = c;
	(*buffer_ptr)->gap_start++;
}

void
buffer_delete_char(Buffer *const buffer)
{
	if (buffer->gap_start != 0) {
		buffer->gap_start--;
	}
}

void
buffer_expand_gap_by_page(Buffer **buffer_ptr)
{
	Buffer *old_buffer = *buffer_ptr;
	long page_size = sysconf(_SC_PAGESIZE);
	if (page_size == -1) {
		return;
	}

	size_t new_buffer_size = old_buffer->size + page_size;

	Buffer *const new_buffer =
		mmap(NULL, new_buffer_size, PROT_READ | PROT_WRITE,
		     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (new_buffer == MAP_FAILED) {
		return;
	}

	memcpy(new_buffer, old_buffer, old_buffer->size);

	size_t bytes_to_copy =
		old_buffer->size - sizeof(*old_buffer) - old_buffer->gap_end;
	new_buffer->gap_end =
		(new_buffer_size - sizeof(*old_buffer)) - bytes_to_copy;

	new_buffer->size = new_buffer_size;

	memmove(new_buffer->text + new_buffer->gap_end,
		old_buffer->text + old_buffer->gap_end, bytes_to_copy);

	*buffer_ptr = new_buffer;

	munmap(old_buffer, old_buffer->size);
}
