#include "../internal.h"
#include <mr_utils.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

internal int next_empty_buffer(void);

Buffer *buffers[MAX_BUFFERS] = { NULL };

Err
buffer_create(FILE *file, size_t size, BufferID *new_buffer_id)
{
	int next_free_buffer_space = next_empty_buffer();
	if (next_free_buffer_space == NOT_FOUND) {
		return NOT_FOUND;
	}

	long page_size = sysconf(_SC_PAGESIZE);
	if (page_size == -1) {
		return ERR;
	}
	size = (size + page_size - 1) & ~(page_size - 1);

	Buffer *const new_buffer = mmap(NULL, size, PROT_READ | PROT_WRITE,
					MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (new_buffer == MAP_FAILED) {
		return ERR;
	}

	memset(new_buffer, 0, sizeof(*new_buffer));

	new_buffer->write_to = file;
	new_buffer->size = size;

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

		new_buffer->gap_start = 0;
		new_buffer->gap_end = BUFFER_MAX_TEXT_LENGTH(size) - length;

		fread(new_buffer->text + new_buffer->gap_end, 1, length, file);
	} else {
		memset(new_buffer->text, 0, BUFFER_MAX_TEXT_LENGTH(size));
		new_buffer->gap_start = 0;
		new_buffer->gap_end = BUFFER_MAX_TEXT_LENGTH(size);
	}

	buffers[next_free_buffer_space] = new_buffer;
	*new_buffer_id = next_free_buffer_space;

	return OK;
}

internal int
next_empty_buffer(void)
{
	for (uint8_t i = 0; i < MAX_BUFFERS; i++) {
		if (buffers[i] == NULL) {
			return i;
		}
	}

	return NOT_FOUND;
}

void
buffer_destroy(BufferID buffer_id)
{
	munmap(buffers[buffer_id], buffers[buffer_id]->size);
	buffers[buffer_id] = NULL;
}

void
buffer_move_gap(BufferID buffer_id, size_t gap_start)
{
	Buffer *buffer = buffers[buffer_id];
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
buffer_insert_char(BufferID buffer_id, char c)
{
	if (buffers[buffer_id]->gap_start == buffers[buffer_id]->gap_end) {
		buffer_expand_gap_by_page(buffer_id);
	}

	Buffer *buffer = buffers[buffer_id];
	*(buffer->text + buffer->gap_start) = c;
	buffer->gap_start++;
}

void
buffer_delete_char(BufferID buffer_id)
{
	if (buffers[buffer_id]->gap_start != 0) {
		buffers[buffer_id]->gap_start--;
	}
}

void
buffer_expand_gap_by_page(BufferID buffer_id)
{
	Buffer *old_buffer = buffers[buffer_id];
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
		BUFFER_MAX_TEXT_LENGTH(old_buffer->size) - old_buffer->gap_end;
	new_buffer->gap_end =
		BUFFER_MAX_TEXT_LENGTH(new_buffer_size) - bytes_to_copy;

	new_buffer->size = new_buffer_size;

	memmove(new_buffer->text + new_buffer->gap_end,
		old_buffer->text + old_buffer->gap_end, bytes_to_copy);

	buffers[buffer_id] = new_buffer;

	munmap(old_buffer, old_buffer->size);
}
