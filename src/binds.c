#include "internal.h"

void
editor_debug_dump_buffer_to_file(struct clk_EditorState *state)
{
	debug_save_buffer_to_file(buffers[state->current_buffer.buffer],
				  "./buffer_dump.txt");
	return;
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
editor_buffer_backspace(struct clk_EditorState *state)
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
editor_click_within_current_buffer(struct clk_EditorState *state)
{
	const uint16_t mouse_x_pos = clicker_event.mouse.x;
	const uint16_t mouse_y_pos = clicker_event.mouse.y;

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

	uint16_t row =
		floorf((float)mouse_y_pos - origin_y) / (float)font_height;
	uint16_t col =
		floorf((float)mouse_x_pos - origin_x) / (float)font_width;

	size_t max_row = buffer_get_max_row(state->current_buffer.buffer);
	if (row > max_row) {
		row = max_row;
	}

	char *ptr = buffer_get_ptr_of_line(state->current_buffer.buffer, row);

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

	if (col > col_count) {
		col = col_count;
	}

	editor_set_cursor_position(state, row, col);

	buffer_move_gap_to_row_col(state->current_buffer.buffer,
				   state->current_buffer.cursor.row,
				   state->current_buffer.cursor.col,
				   state->tab_spaces);
}

mrm_internal void
editor_increase_current_buffer_text_size(struct clk_EditorState *state)
{
	state->current_buffer.font_size *= 1.2;
	return;
}

mrm_internal void
editor_decrease_current_buffer_text_size(struct clk_EditorState *state)
{
	state->current_buffer.font_size /= 1.2;
	return;
}

const struct clk_BindDefine clicker_binds[CLK_BIND_COUNT] = {
	{
		.type = CLK_WINDOW_EVENT_TYPE_KEYDOWN,
		.event.keyboard_event = { FALSE, "", CLK_KEYSYM_DEBUG_BIND },
		.on_event = &editor_debug_dump_buffer_to_file,
	},

	{
		.type = CLK_WINDOW_EVENT_TYPE_KEYDOWN,
		.event.keyboard_event = { TRUE, "", CLK_KEYSYM_EQUAL },
		.on_event = &editor_increase_current_buffer_text_size,
	},

	{
		.type = CLK_WINDOW_EVENT_TYPE_KEYDOWN,
		.event.keyboard_event = { TRUE, "", CLK_KEYSYM_MINUS },
		.on_event = &editor_decrease_current_buffer_text_size,
	},

	{
		.type = CLK_WINDOW_EVENT_TYPE_KEYDOWN,
		.event.keyboard_event = { FALSE, "", CLK_KEYSYM_ARROW_UP },
		.on_event = &editor_current_buffer_move_cursor_up,
	},

	{
		.type = CLK_WINDOW_EVENT_TYPE_KEYDOWN,
		.event.keyboard_event = { FALSE, "", CLK_KEYSYM_ARROW_DOWN },
		.on_event = &editor_current_buffer_move_cursor_down,
	},

	{
		.type = CLK_WINDOW_EVENT_TYPE_KEYDOWN,
		.event.keyboard_event = { FALSE, "", CLK_KEYSYM_ARROW_LEFT },
		.on_event = &editor_current_buffer_move_cursor_left,
	},

	{
		.type = CLK_WINDOW_EVENT_TYPE_KEYDOWN,
		.event.keyboard_event = { FALSE, "", CLK_KEYSYM_ARROW_RIGHT },
		.on_event = &editor_current_buffer_move_cursor_right,
	},

	{
		.type = CLK_WINDOW_EVENT_TYPE_KEYDOWN,
		.event.keyboard_event = { FALSE, "", CLK_KEYSYM_BACKSPACE },
		.on_event = &editor_buffer_backspace,
	},

	{
		.type = CLK_WINDOW_EVENT_TYPE_MOUSEDOWN,
		.event.mouse_event = { CLK_WINDOW_EVENT_MOUSE1, 0, 0 },
		.on_event = &editor_click_within_current_buffer,
	}
};
