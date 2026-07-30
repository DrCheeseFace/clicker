#ifndef INTERNAL_H
#define INTERNAL_H

#include <locale.h>
#include <math.h>
#include <mr_utils.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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
	"https://opensource.org/license/BSD-3-clause\n"                        \
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

// @TODO make this hex?
#define UTF8_RETURN '\n'
#define UTF8_NEWLINE '\r'
#define UTF8_TAB '\t'

//
// TIME
//

#define NS_PER_SEC 1000000000ULL
#define NS_PER_MSEC 1000000ULL
#define NS_PER_USEC 1000ULL

struct clk_Time {
	uint64_t s;
	uint64_t ns;
};

void time_get_time(struct clk_Time *time);

void time_get_delta(struct clk_Time start, struct clk_Time end,
		    struct clk_Time *delta);

// microsecond sleep
void time_sleep_us(uint32_t ms);

Bool time_greater_than_or_equal_to(struct clk_Time t1, struct clk_Time t2);

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

void debug_save_buffer_to_file(Buffer *buffer, const char *filepath);
size_t get_row_length(BufferID bufferid, size_t row, uint8_t tab_spaces);

// -1 for the null terminator :(
#define BUFFER_MAX_TEXT_BYTES_LENGTH(size) ((size) - sizeof(Buffer) - 1)

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

void buffer_move_gap_to_row_col(const BufferID buffer_id, size_t row,
				size_t col, size_t tab_spaces);

void buffer_seek_next_utf8(Buffer *const buffer, char **p);

size_t buffer_get_max_row(const BufferID buffer_id);

#define BUFFERS_GET_BUFFER_BY_ID(idx) (buffers[(idx)])

//
// WINDOW
//

#define WINDOW_BACKGROUND_COLOR 0x00022424

#define WINDOW_BACKGROUND_COLOR_R                                              \
	(((WINDOW_BACKGROUND_COLOR >> 16) & 0xFF) / 255.0)
#define WINDOW_BACKGROUND_COLOR_G                                              \
	(((WINDOW_BACKGROUND_COLOR >> 8) & 0xFF) / 255.0)
#define WINDOW_BACKGROUND_COLOR_B ((WINDOW_BACKGROUND_COLOR & 0xFF) / 255.0)

struct clk_Window {
	void *window_ctx;

	uint16_t window_w;
	uint16_t window_h;
};

enum clk_Keysym {
	CLK_KEYSYM_ARROW_UP = 0,
	CLK_KEYSYM_ARROW_DOWN,
	CLK_KEYSYM_ARROW_LEFT,
	CLK_KEYSYM_ARROW_RIGHT,
	CLK_KEYSYM_EQUAL,
	CLK_KEYSYM_MINUS,
	CLK_KEYSYM_BACKSPACE,
	CLK_KEYSYM_SHIFT_LEFT,
	CLK_KEYSYM_CTRL_LEFT,
	CLK_KEYSYM_DEBUG_BIND,
	CLK_KEYSYM_ESCAPE,
	CLK_KEYSYM_COUNT,
	CLK_KEYSYM_NOT_FOUND,

	CLK_KEYSYM_COUNT_ //@TODO is this sane to do
};

extern const char *clk_keysym_to_string[CLK_KEYSYM_COUNT_];

enum clk_EventMouseButton {
	CLK_WINDOW_EVENT_MOUSE_NONE,
	CLK_WINDOW_EVENT_MOUSE1,
	CLK_WINDOW_EVENT_MOUSE2,
	CLK_WINDOW_EVENT_MOUSE3,
	CLK_WINDOW_EVENT_MOUSE_SCROLL_UP,
	CLK_WINDOW_EVENT_MOUSE_SCROLL_DOWN,

};

enum clk_InputType {
	CLK_INPUT_TYPE_MOUSE,
	CLK_INPUT_TYPE_KEYBOARD,
	CLK_INPUT_TYPE_WINDOW_REQUEST,
};

enum clk_Request { CLK_REQUEST_CLOSE, CLK_REQUEST_RESIZE };

struct clk_Input {
	enum clk_InputType tag;

	struct clk_Time time;

	union {
		enum clk_EventMouseButton mouse_button;

		enum clk_Request window_request;

		struct clk_EventKeyboard {
			// @TODO fuck this bool. fix me lol
			char utf8[8]; // @TODO REMOVE THIS ASS
			enum clk_Keysym keysym;
		} key;

	} input;
};

#define MAX_INPUTS 16

struct clk_Keystate {
	uint8_t inputs_len;

	struct clk_MousePosition {
		uint16_t x;
		uint16_t y;
	} mouse_position;

	struct clk_Input inputs[MAX_INPUTS];
};

extern struct clk_Keystate clicker_keystate;

void window_init(struct clk_Window *window, int window_x, int window_y,
		 int window_w, int window_h, int border_w);

int window_free(struct clk_Window window);

void window_update_window_size(struct clk_Window *window);

// returns 1 if event occured
void window_pol_event(void);

#define INPUT_WINDOW_REQUEST(request_)                                         \
	(struct clk_Input)                                                     \
	{                                                                      \
		.tag = CLK_INPUT_TYPE_WINDOW_REQUEST,                          \
		.input.window_request = (request_)                             \
	}

#define INPUT_KEYBOARD(keysym_)                                                \
	(struct clk_Input)                                                     \
	{                                                                      \
		.tag = CLK_INPUT_TYPE_KEYBOARD, .input.key.keysym = (keysym_)  \
	}

#define INPUT_MOUSE(button_)                                                   \
	(struct clk_Input)                                                     \
	{                                                                      \
		.tag = CLK_INPUT_TYPE_MOUSE, .input.mouse_button = (button_)   \
	}

int window_inputs_contains_input(struct clk_Keystate keystate,
				 struct clk_Input input);

void window_inputs_consume_input(struct clk_Keystate *keystate,
				 struct clk_Input input);

void window_flush_display(struct clk_Window window);

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

void draw_update_text_surface_to_window_size(struct clk_Draw *clicker_draw,
					     struct clk_Window clk_window);

void draw_blit_present(struct clk_Window clk_window);

void draw_push_attr(struct clk_Draw clicker_draw);

void draw_pop_attr(struct clk_Draw clicker_draw);

void draw_set_font_size(struct clk_Draw clicker_draw, double size);

void draw_set_draw_color(struct clk_Draw clicker_draw, double r, double g,
			 double b);

void draw_set_line_width(struct clk_Draw clicker_draw, uint16_t width);

void draw_move_cursor_to(struct clk_Draw clicker_draw, double x, double y);

void draw_relative_move_cursor_to(struct clk_Draw clicker_draw, double x,
				  double y);

void draw_update_font_extents(struct clk_Draw *clicker_draw);

void draw_write_text(struct clk_Draw clicker_draw, const char *text,
		     cairo_text_extents_t *text_extents);

void draw_flush(struct clk_Draw clicker_draw);

void draw_line(struct clk_Draw clicker_draw, uint16_t x1, uint16_t y1,
	       uint16_t x2, uint16_t y2);

void draw_fill_rectangle(struct clk_Draw clicker_draw, uint16_t x, uint16_t y,
			 uint16_t w, uint16_t h, float r, float g, float b,
			 cairo_operator_t operator);

void draw_clip_rectangle(struct clk_Draw clicker_draw, double x, double y,
			 double w, double h);

void draw_color_background(struct clk_Draw clicker_draw);

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

void render_frame(struct clk_Renderer *renderer, struct clk_EditorState state);

//
// EDITOR
//
struct clk_EditorState {
	Bool is_running;

	Bool debug_mode;
	Bool resize_required;

	char *err_str;

	uint8_t tab_spaces;
	struct clk_Time last_tick;
	uint8_t target_frame_ms;

	// @TODO need to abstract this away later when dealing with multiple buffers bleh
	struct {
		BufferID buffer;

		float font_size;
		float frame_origin_x;
		float frame_origin_y;

		struct {
			Bool is_visible;
			size_t row;
			size_t col;
		} cursor;

		size_t view_start_row;
		size_t view_row_count; // number of visible rows
		size_t view_start_column;

	} current_buffer;
};

extern struct clk_EditorState clicker_state;

//@TODO scope these to a scene when this gets too stupid to handle
struct clk_BindDefine {
	uint8_t inputs_len;
	struct clk_Input inputs[MAX_INPUTS];

	void (*on_event)(struct clk_EditorState *);
};

enum clk_Bind {
	CLK_BIND_DEBUG,
	CLK_BIND_INCREASE_FONT_SIZE,
	CLK_BIND_DECREASE_FONT_SIZE,
	CLK_BIND_MOVE_CURSOR_UP,
	CLK_BIND_MOVE_CURSOR_DOWN,
	CLK_BIND_MOVE_CURSOR_LEFT,
	CLK_BIND_MOVE_CURSOR_RIGHT,
	CLK_BIND_BACKSPACE,
	CLK_BIND_BUFFER_CLICK,

	CLK_BIND_COUNT,
};

extern const struct clk_BindDefine clicker_binds[CLK_BIND_COUNT];

void editor_init(struct clk_EditorState *state, const char *filepath);

void editor_free(struct clk_EditorState *state);

void editor_set_cursor_position(struct clk_EditorState *state, uint16_t row,
				uint16_t col);

void editor_simulate(struct clk_EditorState *state,
		     struct clk_Keystate *keystate);

void editor_set_err_msg(struct clk_EditorState *state, const char *err_msg,
			...);

#define EDITOR_FATAL(state, msg) editor_fatal(state, msg, __FILE__, __LINE__)
void editor_fatal(struct clk_EditorState *state, const char *err_msg,
		  const char *filename, int line_number);

void editor_frame_start(struct clk_EditorState *state);

void editor_frame_end(struct clk_EditorState *state);

//
// BUTTON
//

struct clk_Button {
#define max_button_id_str_len 32
	char id_str[max_button_id_str_len];

	struct clk_Rect {
		uint16_t x, y, w, h;
	} box;

	// registerd func to run if button is clicked
	void (*on_click)(struct clk_EditorState *state, void *args);

	// registered func to check if button should be destroyed
	Bool (*destroy_when)(struct clk_EditorState *state);

	// passed args to on_click func
	void *args;
};

// adds button to active buttons to check for every frame
// warning expects strlen(id_str) < max_button_id_str_len
Err button_register_button(const char *id_str, struct clk_Rect box,
			   void (*on_click)(struct clk_EditorState *state,
					    void *args),
			   Bool (*destroy_when)(struct clk_EditorState *state),
			   void *args);

// checks and removes registered buttons if needs to be destroy
void button_registered_buttons_purge_dead(struct clk_EditorState *state);

// checks all active buttons, and runs the most recently created button
Bool button_handle_click(struct clk_EditorState *state,
			 struct clk_MousePosition mouse_position);

Bool is_pointer_within_bounds(struct clk_MousePosition pointer,
			      struct clk_Rect bounds);

#endif
