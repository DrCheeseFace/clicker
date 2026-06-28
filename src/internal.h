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

void buffers_destroy_active_buffers(void);

Err buffer_create_blank(size_t size, BufferID *const new_buffer_id);

Err buffer_create_from_file(FILE *const file, BufferID *const new_buffer_id);

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

// @TODO does this gota be separate
typedef void clk_Window;

enum clk_EventType {
	CLK_WINDOW_EVENT_TYPE_NONE,
	CLK_WINDOW_EVENT_TYPE_KEYDOWN,
	CLK_WINDOW_EVENT_TYPE_KEYUP,
	CLK_WINDOW_EVENT_TYPE_MOUSEDOWN,
	CLK_WINDOW_EVENT_TYPE_MOUSEUP,
	CLK_WINDOW_EVENT_TYPE_MOUSEMOVE,
	CLK_WINDOW_EVENT_TYPE_CLOSEREQ,
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

	union {
		struct {
			enum clk_EventMouse button;
			uint16_t x;
			uint16_t y;
		} mouse;

		uint16_t keycode;

	} val;
};

extern struct clk_Event clicker_event;

void window_init(clk_Window **window, int window_x, int window_y, int window_w,
		 int window_h, int border_w);

int window_free(clk_Window *window);

void window_pol_event(void);

void window_clear(clk_Window *const window);

void window_flush_display(clk_Window *const window);

void window_draw_fill_rectangle(clk_Window *const window, uint16_t x,
				uint16_t y, uint16_t w, uint16_t h);

//
// TEXT
//
struct clk_Text {
	cairo_t *cairo_ctx;
	cairo_surface_t *cairo_surface;

	double current_font_ascent;
	double current_font_descent;
	double current_font_height;
	double current_font_max_x_advance;
	double current_font_max_y_advance;
};

void text_init(struct clk_Text *clicker_text, clk_Window *const window_ctx);

void text_free(struct clk_Text clicker_text);

void text_update_text_surface_to_window_size(struct clk_Text *clicker_text,
					     clk_Window *const window_ctx);

void text_push_attr(struct clk_Text clicker_text);

void text_pop_attr(struct clk_Text clicker_text);

void text_set_font_size(struct clk_Text clicker_text, double size);

void text_set_font_color(struct clk_Text clicker_text, double r, double g,
			 double b);

void text_move_cursor_to(struct clk_Text clicker_text, double x, double y);

void text_relative_move_cursor_to(struct clk_Text clicker_text, double x,
				  double y);

void text_update_font_extents(struct clk_Text *clicker_text);

void text_write_text(struct clk_Text clicker_text, const char *text,
		     cairo_text_extents_t *text_extents);

void text_flush(struct clk_Text clicker_text);

//
// RENDER
//

struct clk_Renderer {
	clk_Window *window_ctx;
	struct clk_Text text_ctx;
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

	char *err_str;

	BufferID current_buffer;
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
