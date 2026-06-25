#include "internal.h"
#include <mr_utils.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
editor_init(struct clk_EditorState *state, const char *filepath)
{
	memset(state, 0, sizeof(*state));

#ifdef DEBUG
	state->debug_mode = TRUE;
#endif //DEBUG

	state->is_running = TRUE;

	state->err_str = NULL;

	buffers_init();

	if (filepath) {
		FILE *file = fopen(filepath, "w+");
		if (file == NULL) {
			editor_set_err_msg(state,
					   "failed to open file of filepath %s",
					   filepath);
		}

		Err err = buffer_create_from_file(file, &state->current_buffer);
		if (err == ERR) {
			editor_set_err_msg(
				state, "failed to create buffer from file %s",
				filepath);
		}

	} else {
		Err err = buffer_create_blank(system_page_size,
					      &state->current_buffer);
		if (err == ERR) {
			editor_set_err_msg(
				state, "failed to create buffer from file %s",
				filepath);
		}
	}
}

void
editor_simulate(struct clk_EditorState *state, struct clk_Event event)
{
	if (event.type == CLK_WINDOW_EVENT_TYPE_CLOSEREQ) {
		state->is_running = FALSE;
	}

	/* if (clicker_event.type == CLK_WINDOW_EVENT_TYPE_KEYDOWN) { */
	/* 	buffer_insert_char(test_buffer, */
	/* 			   clicker_event.val.keycode); */
	/* } */
}

void
editor_free(struct clk_EditorState *state)
{
	if (state->err_str) {
		free(state->err_str);
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
