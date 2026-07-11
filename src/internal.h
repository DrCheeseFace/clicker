#ifndef INTERNAL_H
#define INTERNAL_H

#include <mr_utils.h>
#include <stdint.h>

#include <cairo/cairo.h>

//
// MISC
//

#define VERSION "0.0.1"
#define PROGRAM_NAME "clicker"

#define DEFAULT_FONT "Liberation Mono"

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
Bool utf8_is_continuation_byte(char byte);
void utf8_seek_next(char **ptr);

#define UTF8_BACKSPACE 0x08
// @TODO make this hex?
#define UTF8_FULL_BLOCK "█"
#define UTF8_RETURN '\n'
#define UTF8_NEWLINE '\r'

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

#define BUFFER_MAX_TEXT_BYTES_LENGTH(size) ((size) - sizeof(Buffer))

extern Buffer *buffers[MAX_BUFFERS];
extern size_t system_page_size;

// size rounded up to multiple of pagesize
// populates new_buffer_id with created buffer id
// ERR if memory allocation fails
// NOT_FOUND if free buffer space found
Err buffers_init(void);

void buffers_destroy_active_buffers(void);

Err buffer_create_blank(size_t size, BufferID *const new_buffer_id);

Err buffer_create_from_file(FILE *const file, BufferID *const new_buffer_id);

void buffer_destroy(BufferID buffer_id);

// move buffer gap_start to position of char
void buffer_move_gap_to_utf8_idx(const BufferID buffer_id,
				 const size_t char_idx);

// sets new buffer gap_start
void buffer_move_gap(BufferID buffer_id, size_t gap_start);

// byte index of utf8 char excluding buffer gap
size_t buffer_get_logical_byte_idx_of_utf8_idx(const BufferID buffer_id,
					       size_t char_idx);

// byte index of utf8 char including buffer gap
size_t buffer_get_byte_idx_of_utf8_idx(const BufferID buffer_id,
				       size_t char_idx);

// may update global buffers and reallocate
void buffer_insert_ascii_char(BufferID buffer_id, char c);

// may update global buffers and reallocate
void buffer_insert_utf8(const BufferID buffer_id, const char *c);

void buffer_expand_gap_by_page(BufferID buffer_id);

void buffer_delete_ascii_char(BufferID buffer_id);

void buffer_delete_utf8_char(BufferID buffer_id);

// zero indexed row
void *buffer_get_ptr_of_line(BufferID buffer_id, size_t row);

void buffer_get_row_col_of_utf8(BufferID buffer_id, size_t char_idx,
				size_t *col, size_t *row);

#define BUFFERS_GET_BUFFER_BY_ID(idx) (buffers[(idx)])

//
// WINDOW
//

#define WINDOW_BACKGROUND_COLOR 0x00022424;

struct clk_Window {
	void *window_ctx;

	uint16_t window_w;
	uint16_t window_h;
};

enum clk_EventType {
	CLK_WINDOW_EVENT_TYPE_NONE,
	CLK_WINDOW_EVENT_TYPE_KEYDOWN,
	CLK_WINDOW_EVENT_TYPE_KEYUP,
	CLK_WINDOW_EVENT_TYPE_MOUSEDOWN,
	CLK_WINDOW_EVENT_TYPE_MOUSEUP,
	CLK_WINDOW_EVENT_TYPE_MOUSEMOVE,
	CLK_WINDOW_EVENT_TYPE_CLOSEREQ,
	CLK_WINDOW_EVENT_TYPE_RESIZEREQ,
};

enum clk_EventMouse {
	CLK_WINDOW_EVENT_MOUSE1,
	CLK_WINDOW_EVENT_MOUSE2,
	CLK_WINDOW_EVENT_MOUSE3,
	CLK_WINDOW_EVENT_MOUSE_SCROLL_UP,
	CLK_WINDOW_EVENT_MOUSE_SCROLL_DOWN,

};

struct clk_Event {
	enum clk_EventType type;
	// @TODO fuck this bool. fix me lol
	Bool ctrl_down; // i have sinned...

	union {
		struct {
			enum clk_EventMouse button;
			uint16_t x;
			uint16_t y;
		} mouse;

		struct {
			uint16_t keycode;
			char utf8[8];
		} key;

	} val;
};

extern struct clk_Event clicker_event;

void window_init(struct clk_Window *window, int window_x, int window_y,
		 int window_w, int window_h, int border_w);

int window_free(struct clk_Window window);

void window_update_window_size(struct clk_Window *window);

void window_pol_event(void);

void window_clear(struct clk_Window window);

void window_flush_display(struct clk_Window window);

void window_draw_fill_rectangle(struct clk_Window window, uint16_t x,
				uint16_t y, uint16_t w, uint16_t h);

void window_draw_line(struct clk_Window window, uint16_t x1, uint16_t y1,
		      uint16_t x2, uint16_t y2);

//
// DRAW
//
struct clk_Draw {
	cairo_t *cairo_ctx;
	cairo_surface_t *cairo_surface;

	double current_font_ascent;
	double current_font_descent;
	double current_font_height;
	double current_font_max_x_advance;
	double current_font_max_y_advance;
};

void draw_init(struct clk_Draw *clicker_draw, struct clk_Window clk_window);

void draw_free(struct clk_Draw clicker_draw);

void draw_update_text_surface_to_window_size(struct clk_Draw clicker_draw,
					     struct clk_Window clk_window);

void draw_push_attr(struct clk_Draw clicker_draw);

void draw_pop_attr(struct clk_Draw clicker_draw);

void draw_set_font_size(struct clk_Draw clicker_draw, double size);

void draw_set_font_color(struct clk_Draw clicker_draw, double r, double g,
			 double b);

void draw_move_cursor_to(struct clk_Draw clicker_draw, double x, double y);

void draw_relative_move_cursor_to(struct clk_Draw clicker_draw, double x,
				  double y);

void draw_update_font_extents(struct clk_Draw *clicker_draw);

void draw_write_text(struct clk_Draw clicker_draw, const char *text,
		     cairo_text_extents_t *text_extents);

void draw_flush(struct clk_Draw clicker_draw);

void draw_fill_rectangle(struct clk_Draw clicker_draw, uint16_t x, uint16_t y,
			 uint16_t w, uint16_t h, float r, float g, float b,
			 cairo_operator_t operator);

void draw_clip_rectangle(struct clk_Draw clicker_draw, double x, double y,
			 double w, double h);

//
// RENDER
//

struct clk_Renderer {
	struct clk_Window clk_window;
	struct clk_Draw clk_draw;
};

extern struct clk_Renderer clicker_renderer;

typedef struct clk_EditorState clk_EditorState;

void render_init(struct clk_Renderer *const renderer, int window_x,
		 int window_y, int window_w, int window_h, int border_w);

void render_free(struct clk_Renderer *renderer);

void render_frame(struct clk_Renderer *renderer, struct clk_EditorState *state);

//
// EDITOR
//
struct clk_EditorState {
	Bool is_running;

	Bool debug_mode;
	Bool resize_required;

	char *err_str;

	struct {
		BufferID buffer;

		float font_size;
		float frame_origin_x;
		float frame_origin_y;

		size_t cursor_position;

		size_t view_start_row;
		size_t view_start_column;

	} current_buffer;
};

void editor_init(struct clk_EditorState *state, const char *filepath);

void editor_free(struct clk_EditorState *state);

void editor_simulate(struct clk_EditorState *state, struct clk_Event event);

void editor_set_err_msg(struct clk_EditorState *state, const char *err_msg,
			...);

#define EDITOR_FATAL(state, msg) editor_fatal(state, msg, __FILE__, __LINE__)
void editor_fatal(struct clk_EditorState *state, const char *err_msg,
		  const char *filename, int line_number);

#endif
