#include "./internal.h"
#include <string.h>

internal void render_debug_draw_snack(struct clk_Renderer *renderer);

void
render_init(struct clk_Renderer *renderer, int window_x, int window_y,
	    int window_w, int window_h, int border_w)
{
	window_init(&(renderer->window), window_x, window_y, window_w, window_h,
		    border_w);
}

void
render_free(struct clk_Renderer *renderer)
{
	window_destroy(renderer->window);
}

void
render_frame(struct clk_Renderer *renderer, struct clk_EditorState *state)
{
	window_clear(renderer->window);

	if (state->debug_mode) {
		render_debug_draw_snack(renderer);
	}

	const char *teststr = "is this working";
	window_draw_string(renderer->window, 500, 500, teststr,
			   strlen(teststr));

	window_flush_display(renderer->window);
}

internal void
render_debug_draw_snack(struct clk_Renderer *renderer)
{
	char debug_event_snack_text[128];

	sprintf(debug_event_snack_text,
		"eventtype: %d \nkeybutton_val: %d \nmouse_x: %d \nmouse_y: %d \ntext_len: %d",
		clicker_event.type, clicker_event.val.keycode,
		clicker_event.val.mouse.x, clicker_event.val.mouse.y,
		(uint16_t)(BUFFER_MAX_TEXT_LENGTH(buffers[0]->size) -
			   (buffers[0]->gap_end - buffers[0]->gap_start)));

	int line_height = 20;
	int x = 10;
	int y = 20;

	const char *p = debug_event_snack_text;
	const char *newline_loc;

	while ((newline_loc = strchr(p, '\n')) != NULL) {
		int len = newline_loc - p;

		window_draw_string(renderer->window, x, y, p, len);
		y += line_height;
		p = newline_loc + 1; // skip newline character
	}

	window_draw_string(renderer->window, x, y, p, strlen(p));
}
