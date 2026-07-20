#include "internal.h"
#include <locale.h>
#include <math.h>
#include <mr_utils.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// @TODO hacky impl
mrm_internal Bool
is_typeable_character(struct clk_Event event)
{
	if (event.key.keysym == CLK_KEYSYM_NOT_FOUND && !event.key.ctrl_down) {
		return TRUE;
	}

	return FALSE;
}

mrm_internal void
editor_set_cursor_position(struct clk_EditorState *state, uint16_t row,
			   uint16_t col)
{
	state->current_buffer.cursor.row = row;
	state->current_buffer.cursor.col = col;
	state->current_buffer.cursor.is_visible = TRUE;
}

mrm_internal void
editor_handle_buffer_text_input(struct clk_EditorState *state,
				struct clk_Event event)
{
	buffer_insert_utf8(state->current_buffer.buffer, event.key.utf8);

	if (*event.key.utf8 == UTF8_RETURN || *event.key.utf8 == UTF8_NEWLINE) {
		editor_set_cursor_position(
			state, state->current_buffer.cursor.row + 1, 0);
	} else {
		if (*event.key.utf8 == UTF8_TAB) {
			editor_set_cursor_position(
				state, state->current_buffer.cursor.row,
				state->current_buffer.cursor.col +
					state->tab_spaces);
		} else {
			editor_set_cursor_position(
				state, state->current_buffer.cursor.row,
				state->current_buffer.cursor.col + 1);
		}
	}
}

mrm_internal void
editor_current_buffer_move_cursor_up(struct clk_EditorState *state)
{
	if (state->current_buffer.cursor.row > 0) {
		editor_set_cursor_position(state,
					   state->current_buffer.cursor.row - 1,
					   state->current_buffer.cursor.col);

		size_t row_length =
			get_row_length(state->current_buffer.buffer,
				       state->current_buffer.cursor.row,
				       state->tab_spaces);
		if (row_length < state->current_buffer.cursor.col) {
			editor_set_cursor_position(
				state, state->current_buffer.cursor.row,
				row_length);
		}

		buffer_move_gap_to_row_col(state->current_buffer.buffer,
					   state->current_buffer.cursor.row,
					   state->current_buffer.cursor.col,
					   state->tab_spaces);
	}

	return;
}

mrm_internal void
editor_current_buffer_move_cursor_down(struct clk_EditorState *state)
{
	size_t max_row = buffer_get_max_row(state->current_buffer.buffer);

	if (state->current_buffer.cursor.row < max_row) {
		editor_set_cursor_position(state,
					   state->current_buffer.cursor.row + 1,
					   state->current_buffer.cursor.col);

		size_t row_length =
			get_row_length(state->current_buffer.buffer,
				       state->current_buffer.cursor.row,
				       state->tab_spaces);
		if (row_length < state->current_buffer.cursor.col) {
			editor_set_cursor_position(
				state, state->current_buffer.cursor.row,
				row_length);
		}

		buffer_move_gap_to_row_col(state->current_buffer.buffer,
					   state->current_buffer.cursor.row,
					   state->current_buffer.cursor.col,
					   state->tab_spaces);
	}
	return;
}

mrm_internal void
editor_current_buffer_move_cursor_left(struct clk_EditorState *state)
{
	Buffer *const buffer = buffers[state->current_buffer.buffer];
	if (state->current_buffer.cursor.col > 0) {
		if (buffer->gap_start > 0 &&
		    *(buffer->text + buffer->gap_start - 1) == UTF8_TAB) {
			editor_set_cursor_position(
				state, state->current_buffer.cursor.row,
				state->current_buffer.cursor.col -
					state->tab_spaces);

		} else {
			editor_set_cursor_position(
				state, state->current_buffer.cursor.row,
				state->current_buffer.cursor.col - 1);
		}

		buffer_move_gap_to_row_col(state->current_buffer.buffer,
					   state->current_buffer.cursor.row,
					   state->current_buffer.cursor.col,
					   state->tab_spaces);
	}
	return;
}

mrm_internal void
editor_current_buffer_move_cursor_right(struct clk_EditorState *state)
{
	Buffer *const buffer = buffers[state->current_buffer.buffer];
	size_t row_length = get_row_length(state->current_buffer.buffer,
					   state->current_buffer.cursor.row,
					   state->tab_spaces);

	if (state->current_buffer.cursor.col < row_length) {
		if (buffer->gap_end <
			    BUFFER_MAX_TEXT_BYTES_LENGTH(buffer->size) &&
		    *(buffer->text + buffer->gap_end) == UTF8_TAB) {
			editor_set_cursor_position(
				state, state->current_buffer.cursor.row,
				state->current_buffer.cursor.col +
					state->tab_spaces);

		} else {
			editor_set_cursor_position(
				state, state->current_buffer.cursor.row,
				state->current_buffer.cursor.col + 1);
		}

		buffer_move_gap_to_row_col(state->current_buffer.buffer,
					   state->current_buffer.cursor.row,
					   state->current_buffer.cursor.col,
					   state->tab_spaces);
	}
	return;
}

mrm_internal void
editor_handle_buffer_backspace(struct clk_EditorState *state)
{
	Buffer *const buffer = buffers[state->current_buffer.buffer];

	if (buffer->gap_start == 0) {
		return;
	}

	buffer_delete_utf8_char(state->current_buffer.buffer);

	if (state->current_buffer.cursor.col == 0) {
		if (state->current_buffer.cursor.row > 0) {
			editor_set_cursor_position(
				state, state->current_buffer.cursor.row - 1,
				get_row_length(
					state->current_buffer.buffer,
					state->current_buffer.cursor.row - 1,
					state->tab_spaces));
		}
	} else {
		if (*(buffer->text + buffer->gap_start) == UTF8_TAB) {
			editor_set_cursor_position(
				state, state->current_buffer.cursor.row,
				state->current_buffer.cursor.col -
					state->tab_spaces);
		} else {
			editor_set_cursor_position(
				state, state->current_buffer.cursor.row,
				state->current_buffer.cursor.col - 1);
		}
	}
}

mrm_internal void
editor_click_within_current_buffer(struct clk_EditorState *state,
				   struct clk_Event event)
{
	const uint16_t mouse_x_pos = event.mouse.x;
	const uint16_t mouse_y_pos = event.mouse.y;

	// check bounds of text frame
	if (mouse_x_pos < state->current_buffer.frame_origin_x)
		return;
	if (mouse_x_pos > (clicker_renderer.clk_window.window_w -
			   state->current_buffer.frame_origin_x))
		return;
	if (mouse_y_pos < state->current_buffer.frame_origin_y)
		return;
	if (mouse_y_pos > (clicker_renderer.clk_window.window_h -
			   state->current_buffer.frame_origin_y))
		return;

	// move cursor within text frame
	const float origin_x = state->current_buffer.frame_origin_x;
	const float origin_y = state->current_buffer.frame_origin_y;

	const double font_height =
		clicker_renderer.clk_draw.current_font_height;
	const double font_width =
		clicker_renderer.clk_draw.current_font_max_x_advance;

	state->current_buffer.cursor.col =
		floorf((float)mouse_x_pos - origin_x) / (float)font_width;
	state->current_buffer.cursor.row =
		floorf((float)mouse_y_pos - origin_y) / (float)font_height;

	size_t max_row = buffer_get_max_row(state->current_buffer.buffer);
	if (state->current_buffer.cursor.row > max_row) {
		editor_set_cursor_position(state, max_row,
					   state->current_buffer.cursor.col);
	}

	char *ptr = buffer_get_ptr_of_line(state->current_buffer.buffer,
					   state->current_buffer.cursor.row);

	Buffer *const buffer = buffers[state->current_buffer.buffer];

	size_t col_count = 0;
	while (ptr <
	       buffer->text + BUFFER_MAX_TEXT_BYTES_LENGTH(buffer->size)) {
		if (*ptr == UTF8_NEWLINE || *ptr == UTF8_RETURN) {
			break;
		}
		buffer_seek_next_utf8(buffer, &ptr);
		col_count++;
	}

	if (state->current_buffer.cursor.col > col_count) {
		editor_set_cursor_position(
			state, state->current_buffer.cursor.row, col_count);
	}

	buffer_move_gap_to_row_col(state->current_buffer.buffer,
				   state->current_buffer.cursor.row,
				   state->current_buffer.cursor.col,
				   state->tab_spaces);
}

// @TODO refactr this hoe
mrm_internal void
editor_handle_current_buffer_input(struct clk_EditorState *state,
				   struct clk_Event event)
{
	if (event.type == CLK_WINDOW_EVENT_TYPE_KEYDOWN) {
		switch (event.key.keysym) {
		case CLK_KEYSYM_BACKSPACE: {
			editor_handle_buffer_backspace(state);
			return;
		}

		case CLK_KEYSYM_ARROW_UP:
			editor_current_buffer_move_cursor_up(state);
			return;

		case CLK_KEYSYM_ARROW_DOWN:
			editor_current_buffer_move_cursor_down(state);
			return;

		case CLK_KEYSYM_ARROW_LEFT:
			editor_current_buffer_move_cursor_left(state);
			return;

		case CLK_KEYSYM_ARROW_RIGHT:
			editor_current_buffer_move_cursor_right(state);
			return;

		case CLK_KEYSYM_ADD: {
			if (event.key.ctrl_down) {
				state->current_buffer.font_size *= 1.2;
			}

			break;
		}

		case CLK_KEYSYM_MINUS: {
			if (event.key.ctrl_down) {
				state->current_buffer.font_size /= 1.2;
			}

			break;
		}

		default: {
			// @TODO function to check if utf8 is actgually text and allat
			if (is_typeable_character(event)) {
				editor_handle_buffer_text_input(state, event);
			}
			return;
		}
		}
	}

	if (event.type == CLK_WINDOW_EVENT_TYPE_MOUSEDOWN) {
		switch (event.mouse.button) {
		case CLK_WINDOW_EVENT_MOUSE1: {
			editor_click_within_current_buffer(state, event);
			return;
		}

		default: {
			return;
		}
		}
	}
}

// @TODO remove me
mrm_internal const struct clk_Time toggle_visibility_delay = {
	.s = 0,
	.ns = 700000000ULL
};

mrm_internal void
editor_blink_cursor(struct clk_EditorState *state)
{
	// doesnt have to set to "toggle_visibility_delay", its just a convininent constant
	local_persist struct clk_Time last_is_visible_toggle =
		toggle_visibility_delay;

	struct clk_Time delta;
	time_get_delta(last_is_visible_toggle, state->last_tick, &delta);
	if (delta.s >= toggle_visibility_delay.s &&
	    delta.ns >= toggle_visibility_delay.ns) {
		last_is_visible_toggle = state->last_tick;
		state->current_buffer.cursor.is_visible =
			!state->current_buffer.cursor.is_visible;
	}
}

void
editor_init(struct clk_EditorState *state, const char *filepath)
{
	memset(state, 0, sizeof(*state));

#ifdef DEBUG
	state->debug_mode = TRUE;
#endif //DEBUG

	state->is_running = TRUE;

	state->err_str = NULL;

	state->tab_spaces = 8;

	time_get_time(&state->last_tick);
	state->target_frame_ms = 16;

	state->current_buffer.font_size = 40;
	state->current_buffer.frame_origin_x = 50;
	state->current_buffer.frame_origin_y = 50;

	state->current_buffer.cursor.is_visible = TRUE;
	editor_set_cursor_position(state, 0, 0);

	state->current_buffer.view_start_row = 0;
	state->current_buffer.view_start_column = 0;

	setlocale(LC_ALL, "");

	buffers_init();

	if (filepath) {
		FILE *file = fopen(filepath, "w+");
		if (file == NULL) {
			editor_set_err_msg(state,
					   "failed to open file of filepath %s",
					   filepath);
		}

		Err err = buffer_create_from_file(
			file, &state->current_buffer.buffer);
		if (err == ERR) {
			editor_set_err_msg(
				state, "failed to create buffer from file %s",
				filepath);
		}

	} else {
		Err err = buffer_create_blank(system_page_size,
					      &state->current_buffer.buffer);
		if (err == ERR) {
			editor_set_err_msg(state,
					   "failed to create blank buffer");
		}
	}
}

void
editor_free(struct clk_EditorState *state)
{
	if (state->err_str) {
		free(state->err_str);
	}

	buffers_destroy_active_buffers();
}

void
editor_simulate(struct clk_EditorState *state, struct clk_Event event)
{
	if (state->debug_mode) {
		if (event.key.keysym == CLK_KEYSYM_DEBUG_BIND) {
			debug_save_buffer_to_file(
				buffers[state->current_buffer.buffer],
				"./buffer_dump.txt");
			return;
		}
	}

	state->resize_required = event.type == CLK_WINDOW_EVENT_TYPE_RESIZEREQ;

	state->current_buffer.view_row_count =
		((clicker_renderer.clk_window.window_h -
		  state->current_buffer.frame_origin_y) /
		 clicker_renderer.clk_draw.current_font_height) +
		1;

	if (event.type == CLK_WINDOW_EVENT_TYPE_CLOSEREQ) {
		state->is_running = FALSE;
	}

	editor_handle_current_buffer_input(state, event);

	editor_blink_cursor(state);
}

void
editor_frame_start(struct clk_EditorState *state)
{
	time_get_time(&state->last_tick);
}

void
editor_frame_end(struct clk_EditorState *state)
{
	struct clk_Time current_time;
	time_get_time(&current_time);

	uint64_t elapsed_ns =
		(current_time.s - state->last_tick.s) * 1000000000ULL +
		current_time.ns - state->last_tick.ns;

	uint64_t target_ns = (uint64_t)state->target_frame_ms * 1000000ULL;

	if (elapsed_ns < target_ns) {
		uint64_t remaining_ns = target_ns - elapsed_ns;
		time_sleep_us((uint32_t)(remaining_ns / 1000ULL));
	}
}

void
editor_set_err_msg(struct clk_EditorState *state, const char *fmt, ...)
{
	if (state->err_str) {
		free(state->err_str);
		state->err_str = NULL;
	}

	char err_buffer[256];

	va_list args;
	va_start(args, fmt);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
	vsprintf(err_buffer, fmt, args);
#pragma GCC diagnostic pop
	va_end(args);

	state->err_str = malloc(strlen(err_buffer));
	if (state->err_str == NULL) {
		EDITOR_FATAL(state,
			     "failed to allocate memory for error message");
	}

	strcpy(state->err_str, err_buffer);

	if (state->debug_mode) {
		exit(1);
	}
}

void
editor_fatal(struct clk_EditorState *state, const char *err_msg,
	     const char *filename, int line_number)
{
	char error[INT8_MAX];
	sprintf(error, "ERR: %s:%d %s", filename, line_number, err_msg);
	perror(error);

	if (state->debug_mode) {
		exit(1);
	}
}
