#include "../internal.h"
#include "./x11_internal.h"

void
text_init(struct clk_Text *clicker_text, struct clk_Window clk_window)
{
	struct x11_Window *x11_window = clk_window.window_ctx;

	XWindowAttributes attributes;
	XGetWindowAttributes(x11_window->main_display, x11_window->main_window,
			     &attributes);

	clicker_text->cairo_surface = cairo_xlib_surface_create(
		x11_window->main_display, x11_window->main_window,
		DefaultVisual(x11_window->main_display,
			      DefaultScreen(x11_window->main_display)),
		attributes.width, attributes.height);

	clicker_text->cairo_ctx = cairo_create(clicker_text->cairo_surface);

	cairo_select_font_face(clicker_text->cairo_ctx, DEFAULT_FONT,
			       CAIRO_FONT_SLANT_NORMAL,
			       CAIRO_FONT_WEIGHT_NORMAL);

	cairo_font_extents_t fontext;
	cairo_font_extents(clicker_text->cairo_ctx, &fontext);
	clicker_text->current_font_ascent = fontext.ascent;
	clicker_text->current_font_descent = fontext.descent;
	clicker_text->current_font_height = fontext.height;
	clicker_text->current_font_max_x_advance = fontext.max_x_advance;
	clicker_text->current_font_max_y_advance = fontext.max_y_advance;
}

void
text_update_text_surface_to_window_size(struct clk_Text clicker_text,
					struct clk_Window clk_window)
{
	cairo_xlib_surface_set_size(clicker_text.cairo_surface,
				    clk_window.window_w, clk_window.window_h);
}

void
text_free(struct clk_Text clicker_text)
{
	cairo_destroy(clicker_text.cairo_ctx);
	cairo_surface_destroy(clicker_text.cairo_surface);
}

void
text_push_attr(struct clk_Text clicker_text)
{
	cairo_save(clicker_text.cairo_ctx);
}

void
text_pop_attr(struct clk_Text clicker_text)
{
	cairo_restore(clicker_text.cairo_ctx);
}

void
text_set_font_size(struct clk_Text clicker_text, double size)
{
	cairo_set_font_size(clicker_text.cairo_ctx, size);
}

void
text_set_font_color(struct clk_Text clicker_text, double r, double g, double b)
{
	cairo_set_source_rgb(clicker_text.cairo_ctx, r, g, b);
}

void
text_move_cursor_to(struct clk_Text clicker_text, double x, double y)
{
	cairo_move_to(clicker_text.cairo_ctx, x, y);
}

void
text_relative_move_cursor_to(struct clk_Text clicker_text, double x, double y)
{
	cairo_rel_move_to(clicker_text.cairo_ctx, x, y);
}

void
text_update_font_extents(struct clk_Text *clicker_text)
{
	cairo_font_extents_t fontext;
	cairo_font_extents(clicker_text->cairo_ctx, &fontext);

	clicker_text->current_font_ascent = fontext.ascent;
	clicker_text->current_font_descent = fontext.descent;
	clicker_text->current_font_height = fontext.height;
	clicker_text->current_font_max_x_advance = fontext.max_x_advance;
	clicker_text->current_font_max_y_advance = fontext.max_y_advance;
}

void
text_write_text(struct clk_Text clicker_text, const char *text,
		cairo_text_extents_t *text_extents)
{
	if (text_extents != NULL) {
		cairo_text_extents(clicker_text.cairo_ctx, text, text_extents);
	}

	cairo_show_text(clicker_text.cairo_ctx, text);
}

void
text_flush(struct clk_Text clicker_text)
{
	cairo_surface_flush(clicker_text.cairo_surface);
}
