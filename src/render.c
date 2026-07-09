#include "./internal.h"
#include <string.h>

internal void render_debug_draw_snack(struct clk_Renderer renderer);
internal void render_text_buffer(struct clk_Renderer *renderer,
				 struct clk_EditorState state);

void
render_init(struct clk_Renderer *renderer, int window_x, int window_y,
	    int window_w, int window_h, int border_w)
{
	window_init(&(renderer->clk_window), window_x, window_y, window_w,
		    window_h, border_w);

	text_init(&(renderer->clk_text), renderer->clk_window);
}

void
render_free(struct clk_Renderer *renderer)
{
	text_free(renderer->clk_text);
	window_free(renderer->clk_window);
}

void
render_frame(struct clk_Renderer *renderer, struct clk_EditorState *const state)
{
	window_clear(renderer->clk_window);

	if (state->resize_required) {
		window_update_window_size(&renderer->clk_window);
		text_update_text_surface_to_window_size(renderer->clk_text,
							renderer->clk_window);

		state->resize_required = FALSE;
	}

	if (state->debug_mode) {
		render_debug_draw_snack(*renderer);
	}

	render_text_buffer(renderer, *state);

	text_flush(renderer->clk_text);
	window_flush_display(renderer->clk_window);
}

internal void
render_text_buffer(struct clk_Renderer *renderer, struct clk_EditorState state)
{
	const float font_size = state.current_buffer.font_size;
	text_push_attr(renderer->clk_text);

	// clip box for text
	cairo_rectangle(renderer->clk_text.cairo_ctx,
			state.current_buffer.frame_origin_x,
			state.current_buffer.frame_origin_y,
			renderer->clk_window.window_w -
				(state.current_buffer.frame_origin_x * 2),
			renderer->clk_window.window_h -
				(state.current_buffer.frame_origin_y * 2));
	cairo_clip(renderer->clk_text.cairo_ctx);

	text_set_font_size(renderer->clk_text, font_size);
	text_set_font_color(renderer->clk_text, 1, 1, 1);
	text_update_font_extents(&renderer->clk_text);

	size_t start_row = state.current_buffer.view_start_row;
	if (state.current_buffer.view_start_row != 0) {
		// no + text_height offset to partially render line above view of buffer
		start_row--;
		text_move_cursor_to(renderer->clk_text,
				    state.current_buffer.frame_origin_x,
				    state.current_buffer.frame_origin_y);
	} else {
		text_move_cursor_to(
			renderer->clk_text, state.current_buffer.frame_origin_x,
			state.current_buffer.frame_origin_y +
				renderer->clk_text.current_font_ascent);
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
				    renderer->clk_text.current_font_height) +
				   1;

	char *end_p = buffer_get_ptr_of_line(state.current_buffer.buffer,
					     start_row + view_height);

	char *ptr = start_p;

	Buffer *const buffer = buffers[state.current_buffer.buffer];

	while (start_p < end_p) {
		if (ptr == buffer->text + buffer->gap_start) {
			// display to screen but dont make a newline
			*(buffer->text + buffer->gap_start) = '\0';

			text_write_text(renderer->clk_text, start_p, &extents);

			start_p = buffer->text + buffer->gap_end;
			ptr = start_p;

			continue;
		}

		if (*ptr == UTF8_RETURN || *ptr == UTF8_NEWLINE) {
			char orig_char = *ptr;
			*ptr = '\0';

			if (strlen(start_p) != 0) {
				text_write_text(renderer->clk_text, start_p,
						&extents);

				text_relative_move_cursor_to(
					renderer->clk_text, -extents.x_advance,
					renderer->clk_text.current_font_height);
			} else {
				text_relative_move_cursor_to(
					renderer->clk_text, 0,
					renderer->clk_text.current_font_height);
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

	text_pop_attr(renderer->clk_text);
}

internal void
render_debug_draw_snack(struct clk_Renderer renderer)
{
	text_push_attr(renderer.clk_text);

	char debug_event_snack_text[128];
	sprintf(debug_event_snack_text,
		"eventtype: %d \n"
		"keycode: %d \n"
		"utf8: %s \n"
		"utf8_hex: 0x%.8x \n"
		"mouse_x: %d \n"
		"mouse_y: %d \n"
		"text_len: %d",
		clicker_event.type, clicker_event.val.key.keycode,
		clicker_event.val.key.utf8, *clicker_event.val.key.utf8,
		clicker_event.val.mouse.x, clicker_event.val.mouse.y,
		(uint16_t)(BUFFER_MAX_TEXT_BYTES_LENGTH(buffers[0]->size) -
			   (buffers[0]->gap_end - buffers[0]->gap_start)));

	text_set_font_size(renderer.clk_text, 20.0f);
	text_set_font_color(renderer.clk_text, 1, 1, 1);
	text_update_font_extents(&renderer.clk_text);

	text_move_cursor_to(renderer.clk_text, 10.0,
			    10.0 + renderer.clk_text.current_font_ascent);

	const char *p = debug_event_snack_text;
	const char *newline_loc;
	char text_buffer[64];
	cairo_text_extents_t extents;

	while ((newline_loc = strchr(p, '\n')) != NULL) {
		int len = newline_loc - p;

		memcpy(text_buffer, p, len);
		text_buffer[len] = '\0';

		text_write_text(renderer.clk_text, text_buffer, &extents);

		text_relative_move_cursor_to(
			renderer.clk_text, -extents.x_advance,
			renderer.clk_text.current_font_height);

		p = newline_loc + 1;
	}

	memcpy(text_buffer, p, strlen(p));
	text_buffer[strlen(p)] = '\0';
	text_write_text(renderer.clk_text, text_buffer, NULL);

	text_pop_attr(renderer.clk_text);

	// draw wireframe whole window
	window_draw_line(renderer.clk_window, 0, 0,
			 renderer.clk_window.window_w,
			 renderer.clk_window.window_h);

	window_draw_line(renderer.clk_window, renderer.clk_window.window_w, 0,
			 0, renderer.clk_window.window_h);
}
