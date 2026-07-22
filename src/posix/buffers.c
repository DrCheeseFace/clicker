#include "../internal.h"
#include "./posix_internal.h"

Buffer *buffers[MAX_BUFFERS] = { NULL };
size_t system_page_size;

mrm_internal int
next_empty_buffer(void)
{
	for (uint8_t i = 0; i < MAX_BUFFERS; i++) {
		if (buffers[i] == NULL) {
			return i;
		}
	}

	return NOT_FOUND;
}

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
		length = BUFFER_MAX_TEXT_BYTES_LENGTH(size);
	}

	new_buffer->gap_start = 0;
	new_buffer->gap_end = BUFFER_MAX_TEXT_BYTES_LENGTH(size) - length;

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

	// +1 to include the null terminator
	memset(new_buffer->text, 0, BUFFER_MAX_TEXT_BYTES_LENGTH(size) + 1);

	new_buffer->gap_start = 0;
	new_buffer->gap_end = BUFFER_MAX_TEXT_BYTES_LENGTH(size);

	new_buffer->write_to = NULL;
	new_buffer->size = size;

	buffers[next_free_buffer_space] = new_buffer;
	*new_buffer_id = next_free_buffer_space;

	return OK;
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
	size_t gap_start =
		buffer_get_logical_byte_idx_of_utf8_idx(buffer_id, char_idx);
	buffer_move_gap(buffer_id, gap_start);
}

void
buffer_move_gap_to_row_col(const BufferID buffer_id, size_t row, size_t col,
			   size_t tab_spaces)
{
	Buffer *const buffer = buffers[buffer_id];

	char *p = buffer->text;
	if (buffer->gap_start == 0) {
		p = buffer->text + buffer->gap_end;
	}

	while (row > 0) {
		if (*p == UTF8_RETURN || *p == UTF8_NEWLINE) {
			row--;
		}

		buffer_seek_next_utf8(buffer, &p);
	}

	while (col > 0) {
		if (*p == UTF8_RETURN || *p == UTF8_NEWLINE) {
			break;
		}
		if (*p == UTF8_TAB) {
			col -= tab_spaces;
		} else {
			col--;
		}
		buffer_seek_next_utf8(buffer, &p);
	}

	size_t gap_start;
	if (p < buffer->text + buffer->gap_start) {
		gap_start = p - buffer->text;
	} else {
		const size_t gap_size = buffer->gap_end - buffer->gap_start;
		gap_start = (p - buffer->text) - gap_size;
	}

	buffer_move_gap(buffer_id, gap_start);
}

void
buffer_move_gap(const BufferID buffer_id, size_t gap_start)
{
	Buffer *const buffer = buffers[buffer_id];

	const size_t gap_size = buffer->gap_end - buffer->gap_start;

	const size_t max_text_bytes =
		BUFFER_MAX_TEXT_BYTES_LENGTH(buffer->size);
	const size_t max_valid_gap_start = max_text_bytes - gap_size;

	if (gap_start > max_valid_gap_start) {
		gap_start = max_valid_gap_start;
	}

	if (buffer->gap_start == gap_start)
		return;

	if (gap_start < buffer->gap_start) {
		// moving left
		const size_t bytes_to_move = buffer->gap_start - gap_start;
		memmove(buffer->text + gap_start + gap_size,
			buffer->text + gap_start, bytes_to_move);
	} else {
		// moving right
		const size_t bytes_to_move = gap_start - buffer->gap_start;
		memmove(buffer->text + buffer->gap_start,
			buffer->text + buffer->gap_start + gap_size,
			bytes_to_move);
	}

	buffer->gap_start = gap_start;
	buffer->gap_end = gap_start + gap_size;

	return;
}

size_t
buffer_get_logical_byte_idx_of_utf8_idx(const BufferID buffer_id,
					size_t char_idx)
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

	size_t max_text_bytes_length =
		BUFFER_MAX_TEXT_BYTES_LENGTH(buffer->size);
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

size_t
buffer_get_byte_idx_of_utf8_idx(const BufferID buffer_id, size_t char_idx)
{
	Buffer *const buffer = buffers[buffer_id];

	size_t current_char_idx = 0;

	size_t byte_idx;
	for (byte_idx = 0; byte_idx < buffer->gap_start; byte_idx++) {
		if (!utf8_is_continuation_byte(*(buffer->text + byte_idx))) {
			if (current_char_idx == char_idx) {
				return byte_idx;
			}

			current_char_idx++;
		}
	}

	const size_t max_text_bytes_length =
		BUFFER_MAX_TEXT_BYTES_LENGTH(buffer->size);

	for (byte_idx = buffer->gap_end; byte_idx < max_text_bytes_length;
	     byte_idx++) {
		if (!utf8_is_continuation_byte(*(buffer->text + byte_idx))) {
			if (current_char_idx == char_idx) {
				return byte_idx;
			}

			current_char_idx++;
		}
	}

	if (current_char_idx == char_idx) {
		return byte_idx;
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
buffer_delete_ascii_char(const BufferID buffer_id)
{
	Buffer *const buffer = buffers[buffer_id];

	if (buffer->gap_start != 0) {
		buffer->gap_start--;
	}
}

void
buffer_delete_utf8_char(BufferID buffer_id)
{
	Buffer *const buffer = buffers[buffer_id];

	if (buffer->gap_start != 0) {
		buffer->gap_start--;

		while (utf8_is_continuation_byte(
			*(buffer->text + buffer->gap_start))) {
			buffer->gap_start--;
		}
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
		BUFFER_MAX_TEXT_BYTES_LENGTH(old_buffer->size) -
		old_buffer->gap_end;
	new_buffer->gap_end =
		BUFFER_MAX_TEXT_BYTES_LENGTH(new_buffer_size) - bytes_to_copy;

	new_buffer->size = new_buffer_size;

	*(new_buffer->text + BUFFER_MAX_TEXT_BYTES_LENGTH(new_buffer->size) +
	  1) = '\0';

	memmove(new_buffer->text + new_buffer->gap_end,
		old_buffer->text + old_buffer->gap_end, bytes_to_copy);

	buffers[buffer_id] = new_buffer;

	munmap(old_buffer, old_buffer->size);
}

void *
buffer_get_ptr_of_line(BufferID buffer_id, size_t row)
{
	// @TODO.
	// handle utf-8 possibly some part of utf-8 similar to newline
	Buffer *const buffer = buffers[buffer_id];

	// find first char that isnt a in buffer gap
	char *p = buffer->text;
	if (buffer->gap_start == 0) {
		p = buffer->text + buffer->gap_end;
	}

	size_t current_line = 0;

	size_t max_text_bytes = BUFFER_MAX_TEXT_BYTES_LENGTH(buffer->size);

	while (current_line != row && p < buffer->text + max_text_bytes) {
		if (*p == UTF8_RETURN || *p == UTF8_NEWLINE) {
			current_line++;
		}

		buffer_seek_next_utf8(buffer, &p);
	}

	return p;
}

void
buffer_seek_next_utf8(Buffer *const buffer, char **p)
{
	utf8_seek_next(p);

	if (*p == buffer->text + buffer->gap_start) {
		*p = buffer->text + buffer->gap_end;
		return;
	}
}

size_t
buffer_get_max_row(const BufferID buffer_id)
{
	Buffer *const buffer = buffers[buffer_id];
	size_t max_row = 0;

	char *p = buffer->text;
	if (buffer->gap_start == 0) {
		p = buffer->text + buffer->gap_end;
	}

	while (p < buffer->text + BUFFER_MAX_TEXT_BYTES_LENGTH(buffer->size)) {
		if (*p == UTF8_RETURN || *p == UTF8_NEWLINE) {
			max_row++;
		}
		buffer_seek_next_utf8(buffer, &p);
	}

	return max_row;
}
