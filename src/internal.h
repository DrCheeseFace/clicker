#ifndef INTERNAL_H
#define INTERNAL_H

#include <mr_utils.h>
#include <stdint.h>

//
// MISC
//

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

//
// BUFFER
//

#define MAX_BUFFERS UINT8_MAX
typedef uint8_t BufferID;

typedef struct {
	FILE *write_to;

	size_t size;

	size_t gap_start;
	size_t gap_end;

	char text[];
} Buffer;

#define BUFFER_MAX_TEXT_LENGTH(size) ((size) - sizeof(Buffer))

extern Buffer *buffers[MAX_BUFFERS];
extern size_t system_page_size;

// size rounded up to multiple of pagesize
// populates new_buffer_id with created buffer id
// ERR if memory allocation fails
// NOT_FOUND if free buffer space found
Err buffers_init(void);
Err buffer_create(FILE *const file, size_t size, BufferID *const new_buffer_id);
void buffer_destroy(BufferID buffer_id);

void buffer_move_gap(BufferID buffer_id, size_t gap_start);

// may update global buffers and reallocate
void buffer_insert_char(BufferID buffer_id, char c);

void buffer_expand_gap_by_page(BufferID buffer_id);

void buffer_delete_char(BufferID buffer_id);

#define BUFFERS_GET_BUFFER_BY_ID(idx) (buffers[(idx)])

//
// WINDOW
//

#define WINDOW_BACKGROUND_COLOR 0x00022424;

typedef void clk_Window;

enum clk_WindowEventType {
	CLK_WINDOW_EVENT_TYPE_NONE,
	CLK_WINDOW_EVENT_TYPE_KEYDOWN,
	CLK_WINDOW_EVENT_TYPE_KEYUP,
	CLK_WINDOW_EVENT_TYPE_MOUSEDOWN,
	CLK_WINDOW_EVENT_TYPE_MOUSEUP,
	CLK_WINDOW_EVENT_TYPE_MOUSEMOVE,
	CLK_WINDOW_EVENT_TYPE_CLOSEREQ,
};

enum clk_WindowEventMouse {
	CLK_WINDOW_EVENT_MOUSE1,
	CLK_WINDOW_EVENT_MOUSE2,
	CLK_WINDOW_EVENT_MOUSE3,
	CLK_WINDOW_EVENT_MOUSE_SCROLL_UP,
	CLK_WINDOW_EVENT_MOUSE_SCROLL_DOWN,

};

struct clk_WindowEvent {
	enum clk_WindowEventType type;

	union {
		struct {
			enum clk_WindowEventMouse button;
			uint16_t x;
			uint16_t y;
		} mouse;

		uint16_t keycode;

	} val;
};

clk_Window *window_create(int window_x, int window_y, int window_w,
			  int window_h, int border_w);
int window_destroy(clk_Window *const window);

void window_get_event(clk_Window *const window,
		      struct clk_WindowEvent *const event);

void window_clear(clk_Window *const window);

void window_flush_display(clk_Window *const window);

#ifdef DEBUG
void window_draw_debug_snack(clk_Window *const window, const char *text);
#endif

#endif
