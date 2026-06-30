#include "./internal.h"
#include <string.h>

internal void render_debug_draw_snack(struct clk_Renderer renderer);

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
		text_update_text_surface_to_window_size(&renderer->clk_text,
							renderer->clk_window);

		state->resize_required = FALSE;
	}

	if (state->debug_mode) {
		render_debug_draw_snack(*renderer);
	}

	text_flush(renderer->clk_text);
	window_flush_display(renderer->clk_window);
}

internal void
render_debug_draw_snack(struct clk_Renderer renderer)
{
	text_push_attr(renderer.clk_text);

	char debug_event_snack_text[128];
	sprintf(debug_event_snack_text,
		"eventtype: %d \nkeycode: %d \nutf8: %s \nmouse_x: %d \nmouse_y: %d \ntext_len: %d",
		clicker_event.type, clicker_event.val.key.keycode,
		clicker_event.val.key.utf8, clicker_event.val.mouse.x,
		clicker_event.val.mouse.y,
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
