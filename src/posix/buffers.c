#include "../internal.h"
#include <mr_utils.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

internal int next_empty_buffer(void);

Buffer *buffers[MAX_BUFFERS] = { NULL };
size_t system_page_size;

Err
buffers_init(void)
{
	const long size = sysconf(_SC_PAGESIZE);
	if (size == -1) {
		return ERR;
	}

	system_page_size = size;
	return OK;
}

void
buffers_destroy_active_buffers(void)
{
	for (BufferID i = 0; i < MAX_BUFFERS; i++) {
		if (buffers[i]) {
			buffer_destroy(i);
		}
	}
}

Err
buffer_create_from_file(FILE *const file, BufferID *const new_buffer_id)
{
	const int next_free_buffer_space = next_empty_buffer();
	if (next_free_buffer_space == NOT_FOUND) {
		return NOT_FOUND;
	}

	fseek(file, 0, SEEK_END);
	long length = ftell(file);
	fseek(file, 0, SEEK_SET);

	size_t size = (length > 0) ? (size_t)length : 0;

	size = (size + system_page_size - 1) & ~(system_page_size - 1);

	Buffer *const new_buffer = mmap(NULL, size, PROT_READ | PROT_WRITE,
					MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (new_buffer == MAP_FAILED) {
		return ERR;
	}

	memset(new_buffer, 0, sizeof(*new_buffer));

	fseek(file, 0, SEEK_END);
	length = ftell(file);
	fseek(file, 0, SEEK_SET);

	if (length < 0) {
		length = 0;
	}

	if ((size_t)length > size) {
		length = BUFFER_MAX_BYTES_LENGTH(size);
	}

	new_buffer->gap_start = 0;
	new_buffer->gap_end = BUFFER_MAX_BYTES_LENGTH(size) - length;

	fread(new_buffer->text + new_buffer->gap_end, 1, length, file);

	new_buffer->write_to = file;
	new_buffer->size = size;

	buffers[next_free_buffer_space] = new_buffer;
	*new_buffer_id = next_free_buffer_space;

	return OK;
}

Err
buffer_create_blank(size_t size, BufferID *const new_buffer_id)
{
	const int next_free_buffer_space = next_empty_buffer();
	if (next_free_buffer_space == NOT_FOUND) {
		return NOT_FOUND;
	}

	size = (size + system_page_size - 1) & ~(system_page_size - 1);

	Buffer *const new_buffer = mmap(NULL, size, PROT_READ | PROT_WRITE,
					MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (new_buffer == MAP_FAILED) {
		return ERR;
	}

	memset(new_buffer, 0, sizeof(*new_buffer));

	memset(new_buffer->text, 0, BUFFER_MAX_BYTES_LENGTH(size));
	new_buffer->gap_start = 0;
	new_buffer->gap_end = BUFFER_MAX_BYTES_LENGTH(size);

	new_buffer->write_to = NULL;
	new_buffer->size = size;

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
buffer_destroy(const BufferID buffer_id)
{
	munmap(buffers[buffer_id], buffers[buffer_id]->size);
	buffers[buffer_id] = NULL;
}

void
buffer_move_gap_to_utf8_idx(const BufferID buffer_id, const size_t char_idx)
{
	size_t gap_start = buffer_get_byte_idx_of_utf8_idx(buffer_id, char_idx);
	buffer_move_gap(buffer_id, gap_start);
}

void
buffer_move_gap(const BufferID buffer_id, const size_t gap_start)
{
	Buffer *const buffer = buffers[buffer_id];
	if (buffer->gap_start == gap_start)
		return;

	if (gap_start < buffer->gap_start) {
		const size_t bytes_to_move = buffer->gap_start - gap_start;
		memmove(buffer->text + buffer->gap_end - bytes_to_move,
			buffer->text + buffer->gap_start - bytes_to_move,
			bytes_to_move);
		buffer->gap_end = buffer->gap_end - bytes_to_move;
	} else {
		const size_t bytes_to_move = gap_start - buffer->gap_start;
		memmove(buffer->text + buffer->gap_start,
			buffer->text + buffer->gap_end, bytes_to_move);
		buffer->gap_end = buffer->gap_end + bytes_to_move;
	}

	buffer->gap_start = gap_start;
	return;
}

size_t
buffer_get_byte_idx_of_utf8_idx(const BufferID buffer_id, size_t char_idx)
{
	Buffer *const buffer = buffers[buffer_id];

	size_t logical_byte_idx = 0;
	size_t current_char_idx = 0;

	for (size_t byte_idx = 0; byte_idx < buffer->gap_start; byte_idx++) {
		if (!utf8_is_continuation_byte(*(buffer->text + byte_idx))) {
			if (current_char_idx == char_idx) {
				return logical_byte_idx;
			}

			current_char_idx++;
		}

		logical_byte_idx++;
	}

	size_t max_text_bytes_length = BUFFER_MAX_BYTES_LENGTH(buffer->size);
	for (size_t byte_idx = buffer->gap_end;
	     byte_idx < max_text_bytes_length; byte_idx++) {
		if (!utf8_is_continuation_byte(*(buffer->text + byte_idx))) {
			if (current_char_idx == char_idx) {
				return logical_byte_idx;
			}

			current_char_idx++;
		}

		logical_byte_idx++;
	}

	if (current_char_idx == char_idx) {
		return logical_byte_idx;
	}

	ASSERT(strcmp("ATTEMPTED TO GET BYTES INDEX OF CHAR OUT OF RANGE OF TEXT",
		      ".") == 0);

	__builtin_unreachable();
}

void
buffer_insert_ascii_char(const BufferID buffer_id, const char c)
{
	if (buffers[buffer_id]->gap_start == buffers[buffer_id]->gap_end) {
		buffer_expand_gap_by_page(buffer_id);
	}

	Buffer *const buffer = buffers[buffer_id];
	*(buffer->text + buffer->gap_start) = c;
	buffer->gap_start++;
}

void
buffer_insert_utf8(const BufferID buffer_id, const char *c)
{
	Buffer *const buffer = buffers[buffer_id];

	size_t length = strlen(c);
	if (buffer->gap_start + length >= buffer->gap_end) {
		buffer_expand_gap_by_page(buffer_id);
	}

	for (const char *p = c; p < c + length; p++) {
		*(buffer->text + buffer->gap_start) = *p;
		buffer->gap_start++;
	}
}

void
buffer_delete_char(const BufferID buffer_id)
{
	if (buffers[buffer_id]->gap_start != 0) {
		buffers[buffer_id]->gap_start--;
	}
}

void
buffer_expand_gap_by_page(const BufferID buffer_id)
{
	Buffer *const old_buffer = buffers[buffer_id];

	const size_t new_buffer_size = old_buffer->size + system_page_size;

	Buffer *const new_buffer =
		mmap(NULL, new_buffer_size, PROT_READ | PROT_WRITE,
		     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (new_buffer == MAP_FAILED) {
		return;
	}

	memcpy(new_buffer, old_buffer, old_buffer->size);

	const size_t bytes_to_copy =
		BUFFER_MAX_BYTES_LENGTH(old_buffer->size) - old_buffer->gap_end;
	new_buffer->gap_end =
		BUFFER_MAX_BYTES_LENGTH(new_buffer_size) - bytes_to_copy;

	new_buffer->size = new_buffer_size;

	memmove(new_buffer->text + new_buffer->gap_end,
		old_buffer->text + old_buffer->gap_end, bytes_to_copy);

	buffers[buffer_id] = new_buffer;

	munmap(old_buffer, old_buffer->size);
}
