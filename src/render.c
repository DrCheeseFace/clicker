#include "./internal.h"
#include <string.h>

internal void render_debug_draw_snack(struct clk_Renderer *renderer);

void
render_init(struct clk_Renderer *renderer, int window_x, int window_y,
	    int window_w, int window_h, int border_w)
{
	window_init(&(renderer->window_ctx), window_x, window_y, window_w,
		    window_h, border_w);

	text_init(&(renderer->text_ctx), renderer->window_ctx);
}

void
render_free(struct clk_Renderer *renderer)
{
	text_free(renderer->text_ctx);
	window_free(renderer->window_ctx);
}

void
render_frame(struct clk_Renderer *renderer, struct clk_EditorState *state)
{
	window_clear(renderer->window_ctx);

	if (state->debug_mode) {
		render_debug_draw_snack(renderer);
	}

	text_flush(renderer->text_ctx);
	window_flush_display(renderer->window_ctx);
}

internal void
render_debug_draw_snack(struct clk_Renderer *renderer)
{
	text_push_attr(renderer->text_ctx);

	char debug_event_snack_text[128];
	sprintf(debug_event_snack_text,
		"eventtype: %d \nkeybutton_val: %d \nmouse_x: %d \nmouse_y: %d \ntext_len: %d",
		clicker_event.type, clicker_event.val.keycode,
		clicker_event.val.mouse.x, clicker_event.val.mouse.y,
		(uint16_t)(BUFFER_MAX_TEXT_LENGTH(buffers[0]->size) -
			   (buffers[0]->gap_end - buffers[0]->gap_start)));

	text_set_font_size(renderer->text_ctx, 20.0f);
	text_set_font_color(renderer->text_ctx, 1, 1, 1);
	text_update_font_extents(&renderer->text_ctx);

	text_move_cursor_to(renderer->text_ctx, 10.0,
			    10.0 + renderer->text_ctx.current_font_ascent);

	const char *p = debug_event_snack_text;
	const char *newline_loc;
	char text_buffer[64];
	cairo_text_extents_t extents;

	while ((newline_loc = strchr(p, '\n')) != NULL) {
		int len = newline_loc - p;

		memcpy(text_buffer, p, len);
		text_buffer[len] = '\0';

		text_write_text(renderer->text_ctx, text_buffer, &extents);

		text_relative_move_cursor_to(
			renderer->text_ctx, -extents.x_advance,
			renderer->text_ctx.current_font_height);

		p = newline_loc + 1;
	}

	memcpy(text_buffer, p, strlen(p));
	text_buffer[strlen(p)] = '\0';
	text_write_text(renderer->text_ctx, text_buffer, NULL);

	text_pop_attr(renderer->text_ctx);
}
