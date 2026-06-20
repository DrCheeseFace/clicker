#ifndef INTERNAL_H
#define INTERNAL_H

#include <mr_utils.h>
#include <stdint.h>

#define VERSION "0.0.1"
#define PROGRAM_NAME "clicker"

#define LICENSE                                                                \
	"BSD 3-Clause License\n"                                               \
	"Copyright (c) 2026, Tharun Tilakumara\n"                              \
	"https://opensource.org/license/BSD-2-clause\n"                        \
	"\nWritten by Tharun Tilakumara\n"

enum OptionFlags {
	OPTION_FLAGS_HELP_SHORT,
	OPTION_FLAGS_HELP_LONG,
	OPTION_FLAGS_VERSION_SHORT,
	OPTION_FLAGS_VERSION_LONG,
	OPTION_FLAGS_COUNT,
};

void log_help(void);
void log_version(void);
Err process_arg(char *arg);

// NOTE: uint8_max because typedef uint8_t BufferIndex;
#define MAX_BUFFERS UINT8_MAX
typedef uint8_t BufferIndex;

typedef struct {
	FILE *write_to;
	size_t gap_start;
	size_t gap_end;
	size_t cursor_x;
	size_t cursor_y;
	char text[];
} Buffer;

#define DEFAULT_BUFFER_ALLOC_SIZE 1 * 1024 * 1024 // 1mb
#define DEFAULT_BUFFER_MAX_TEXT_LENGTH                                         \
	(DEFAULT_BUFFER_ALLOC_SIZE - sizeof(Buffer))

extern BufferIndex buffer_count;
extern Buffer *buffers[MAX_BUFFERS];

Buffer *buffer_create(FILE *file);
void buffer_destroy(Buffer *buffer);
void buffer_move_gap(Buffer *buffer, size_t gap_start);

void buffer_insert_char(Buffer *buffer, char c);
void buffer_delete_char(Buffer *buffer);

#define BUFFERS_GET_BUFFER_BY_IDX(idx) (buffers[(idx)])

#define WINDOW_BACKGROUND_COLOR 0x00022424;

#endif
