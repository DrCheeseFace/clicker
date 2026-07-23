#include "internal.h"

// @TODO remove me
mrm_internal const struct clk_Time toggle_visibility_delay = {
	.s = 0,
	.ns = 700000000ULL
};

// @TODO hacky impl
mrm_internal void
editor_blink_cursor(struct clk_EditorState *state)
{
	local_persist struct clk_Time last_is_visible_toggle = { 0 };

	struct clk_Time delta;
	time_get_delta(last_is_visible_toggle, state->last_tick, &delta);
	if (delta.s >= toggle_visibility_delay.s &&
	    delta.ns >= toggle_visibility_delay.ns) {
		last_is_visible_toggle = state->last_tick;
		state->current_buffer.cursor.is_visible =
			!state->current_buffer.cursor.is_visible;
	}
}

mrm_internal void
editor_buffer_text_input(struct clk_EditorState *state, struct clk_Event event)
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

void
editor_set_cursor_position(struct clk_EditorState *state, uint16_t row,
			   uint16_t col)
{
	state->current_buffer.cursor.row = row;
	state->current_buffer.cursor.col = col;
	state->current_buffer.cursor.is_visible = TRUE;
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
editor_do_binds(struct clk_EditorState *state, struct clk_Event event)
{
	enum clk_Bind bind = CLK_BIND_DEBUG;

	// skip checking the debug bind in debug mode
	if (!state->debug_mode) {
		bind++;
	}

	while (bind < CLK_BIND_COUNT) {
		struct clk_BindDefine define = clicker_binds[bind];

		switch (define.type) {
		case CLK_WINDOW_EVENT_TYPE_NONE:
			break;

		case CLK_WINDOW_EVENT_TYPE_KEYDOWN: {
			if (define.event.keyboard_event.ctrl_down ==
				    event.key.ctrl_down &&
			    define.event.keyboard_event.keysym ==
				    event.key.keysym) {
				define.on_event(state);
			}
			break;
		}

		case CLK_WINDOW_EVENT_TYPE_KEYUP:
			break;

		case CLK_WINDOW_EVENT_TYPE_MOUSEDOWN: {
			if (define.event.mouse_event.button ==
			    event.mouse.button) {
				define.on_event(state);
			}
			break;
		}

		case CLK_WINDOW_EVENT_TYPE_MOUSEUP:
			break;

		case CLK_WINDOW_EVENT_TYPE_MOUSEMOVE:
			break;

		default: {
			return;
		}
		}

		bind++;
	}
}

void
editor_simulate(struct clk_EditorState *state, struct clk_Event event)
{
	state->resize_required = event.type == CLK_WINDOW_EVENT_TYPE_RESIZEREQ;

	if (event.type == CLK_WINDOW_EVENT_TYPE_CLOSEREQ) {
		state->is_running = FALSE;
		return;
	}

	state->current_buffer.view_row_count =
		((clicker_renderer.clk_window.window_h -
		  state->current_buffer.frame_origin_y) /
		 clicker_renderer.clk_draw.current_font_height) +
		1;

	editor_do_binds(state, event);

	// @TODO hacky fix this be
	if (event.type == CLK_WINDOW_EVENT_TYPE_KEYDOWN &&
	    event.key.keysym == CLK_KEYSYM_NOT_FOUND && !event.key.ctrl_down) {
		editor_buffer_text_input(state, event);
	}

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
