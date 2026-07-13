#include "./internal.h"
#include <string.h>

mrm_internal void render_debug_draw_snack(struct clk_Renderer renderer);
mrm_internal void render_text_buffer(struct clk_Renderer *renderer,
				     struct clk_EditorState state);
mrm_internal void render_text_buffer_cursor(struct clk_Draw clk_draw,
					    struct clk_EditorState state);

void
render_init(struct clk_Renderer *renderer, int window_x, int window_y,
	    int window_w, int window_h, int border_w)
{
	window_init(&(renderer->clk_window), window_x, window_y, window_w,
		    window_h, border_w);

	draw_init(&(renderer->clk_draw), renderer->clk_window);
}

void
render_free(struct clk_Renderer *renderer)
{
	draw_free(renderer->clk_draw);
	window_free(renderer->clk_window);
}

void
render_frame(struct clk_Renderer *renderer, struct clk_EditorState *const state)
{
	window_clear(renderer->clk_window);

	if (state->resize_required) {
		window_update_window_size(&renderer->clk_window);
		draw_update_text_surface_to_window_size(renderer->clk_draw,
							renderer->clk_window);

		state->resize_required = FALSE;
	}

	if (state->debug_mode) {
		render_debug_draw_snack(*renderer);
	}

	render_text_buffer(renderer, *state);

	draw_flush(renderer->clk_draw);
	window_flush_display(renderer->clk_window);
}

mrm_internal void
render_text_buffer(struct clk_Renderer *renderer, struct clk_EditorState state)
{
	const float font_size = state.current_buffer.font_size;
	draw_push_attr(renderer->clk_draw);

	draw_clip_rectangle(renderer->clk_draw,
			    state.current_buffer.frame_origin_x,
			    state.current_buffer.frame_origin_y,
			    renderer->clk_window.window_w -
				    (state.current_buffer.frame_origin_x * 2),
			    renderer->clk_window.window_h -
				    (state.current_buffer.frame_origin_y * 2));

	draw_set_font_size(renderer->clk_draw, font_size);
	draw_set_font_color(renderer->clk_draw, 1, 1, 1);
	draw_update_font_extents(&renderer->clk_draw);

	size_t start_row = state.current_buffer.view_start_row;
	if (state.current_buffer.view_start_row != 0) {
		// no + text_height offset to partially render line above view of buffer
		start_row--;
		draw_move_cursor_to(renderer->clk_draw,
				    state.current_buffer.frame_origin_x,
				    state.current_buffer.frame_origin_y);
	} else {
		draw_move_cursor_to(
			renderer->clk_draw, state.current_buffer.frame_origin_x,
			state.current_buffer.frame_origin_y +
				renderer->clk_draw.current_font_ascent);
	}

	cairo_text_extents_t extents;

	// @TODO impl this when moving view with cursor
	size_t horizontal_offset = state.current_buffer.view_start_column;
	ignore horizontal_offset;

	char *start_p =
		buffer_get_ptr_of_line(state.current_buffer.buffer, start_row);

	// +1 so we can partially see the cutoff line at the bottom
	const size_t view_height = ((renderer->clk_window.window_h -
				     state.current_buffer.frame_origin_y) /
				    renderer->clk_draw.current_font_height) +
				   1;

	char *end_p = buffer_get_ptr_of_line(state.current_buffer.buffer,
					     start_row + view_height);

	char *ptr = start_p;

	Buffer *const buffer = buffers[state.current_buffer.buffer];

	while (start_p < end_p) {
		if (ptr == buffer->text + buffer->gap_start) {
			// display to screen but dont make a newline
			*(buffer->text + buffer->gap_start) = '\0';

			draw_write_text(renderer->clk_draw, start_p, &extents);

			start_p = buffer->text + buffer->gap_end;
			ptr = start_p;

			continue;
		}

		if (*ptr == UTF8_RETURN || *ptr == UTF8_NEWLINE) {
			char orig_char = *ptr;
			*ptr = '\0';

			if (strlen(start_p) != 0) {
				draw_write_text(renderer->clk_draw, start_p,
						&extents);

				draw_relative_move_cursor_to(
					renderer->clk_draw, -extents.x_advance,
					renderer->clk_draw.current_font_height);
			} else {
				draw_relative_move_cursor_to(
					renderer->clk_draw, 0,
					renderer->clk_draw.current_font_height);
			}

			*ptr = orig_char;

			// seek to past newline
			// if past newline into gap, seek to end of gap
			start_p = ptr + 1;
			if (start_p == buffer->text + buffer->gap_start) {
				start_p = buffer->text + buffer->gap_end;
			}
			ptr = start_p;

			continue;
		}

		utf8_seek_next(&ptr);
	}

	draw_pop_attr(renderer->clk_draw);
	render_text_buffer_cursor(renderer->clk_draw, state);
}

// @TODO some static stuff to get blinking working
mrm_internal void
render_text_buffer_cursor(struct clk_Draw clk_draw,
			  struct clk_EditorState state)
{
	const size_t relative_row = state.current_buffer.cursor_position.row -
				    state.current_buffer.view_start_row;

	const size_t relative_col = state.current_buffer.cursor_position.col;

	const size_t origin_y = state.current_buffer.frame_origin_y +
				(relative_row * clk_draw.current_font_height);
	const size_t origin_x =
		state.current_buffer.frame_origin_x +
		(relative_col * clk_draw.current_font_max_x_advance);

	draw_fill_rectangle(clk_draw, origin_x, origin_y,
			    clk_draw.current_font_max_x_advance,
			    clk_draw.current_font_height, 1, 1, 1,
			    CAIRO_OPERATOR_DIFFERENCE);
}

mrm_internal void
render_debug_draw_snack(struct clk_Renderer renderer)
{
	draw_push_attr(renderer.clk_draw);

	char debug_event_snack_text[256];

	snprintf(debug_event_snack_text, sizeof(debug_event_snack_text),
		 "eventtype: %d \n"
		 "keysym: %s \n"
		 "utf8: %s \n"
		 "utf8_hex: 0x%.2hhx \n"
		 "ctrl_down: %d \n"
		 "mouse_button: %d \n"
		 "mouse_x: %d \n"
		 "mouse_y: %d \n"
		 "text_len: %zu\n"
		 "cursor_row: %zu \n"
		 "cursor_col: %zu \n",
		 clicker_event.type,
		 clk_keysym_to_string[clicker_event.key.keysym],
		 clicker_event.key.utf8,
		 (unsigned char)clicker_event.key.utf8[0],
		 clicker_event.key.ctrl_down, clicker_event.mouse.button,
		 clicker_event.mouse.x, clicker_event.mouse.y,
		 (size_t)(BUFFER_MAX_TEXT_BYTES_LENGTH(buffers[0]->size) -
			  (buffers[0]->gap_end - buffers[0]->gap_start)),
		 clicker_state.current_buffer.cursor_position.row,
		 clicker_state.current_buffer.cursor_position.col);

	draw_set_font_size(renderer.clk_draw, 20.0f);
	draw_set_font_color(renderer.clk_draw, 1, 1, 1);
	draw_update_font_extents(&renderer.clk_draw);

	draw_move_cursor_to(renderer.clk_draw, 10.0,
			    10.0 + renderer.clk_draw.current_font_ascent);

	const char *p = debug_event_snack_text;
	const char *newline_loc;
	char text_buffer[256];
	cairo_text_extents_t extents;

	while ((newline_loc = strchr(p, '\n')) != NULL) {
		int len = newline_loc - p;

		memcpy(text_buffer, p, len);
		text_buffer[len] = '\0';

		draw_write_text(renderer.clk_draw, text_buffer, &extents);

		draw_relative_move_cursor_to(
			renderer.clk_draw, -extents.x_advance,
			renderer.clk_draw.current_font_height);

		p = newline_loc + 1;
	}

	memcpy(text_buffer, p, strlen(p));
	text_buffer[strlen(p)] = '\0';

	draw_write_text(renderer.clk_draw, text_buffer, NULL);

	draw_pop_attr(renderer.clk_draw);

	// draw wireframe whole window
	window_draw_line(renderer.clk_window, 0, 0,
			 renderer.clk_window.window_w,
			 renderer.clk_window.window_h);

	window_draw_line(renderer.clk_window, renderer.clk_window.window_w, 0,
			 0, renderer.clk_window.window_h);
}
