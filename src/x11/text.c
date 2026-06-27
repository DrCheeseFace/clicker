#include "../internal.h"
#include "./x11_internal.h"

void
text_init(struct clk_Text *clicker_text, clk_Window *const window_ctx)
{
	struct x11_Window *x11_window = window_ctx;

	XWindowAttributes attributes;
	XGetWindowAttributes(x11_window->main_display, x11_window->main_window,
			     &attributes);

	cairo_surface_t *text_surface = cairo_xlib_surface_create(
		x11_window->main_display, x11_window->main_window,
		DefaultVisual(x11_window->main_display,
			      DefaultScreen(x11_window->main_display)),
		attributes.width, attributes.height);

	clicker_text->cairo_ctx = cairo_create(text_surface);

	cairo_font_extents_t fontext;
	cairo_font_extents(clicker_text->cairo_ctx, &fontext);
	clicker_text->font_ascent = fontext.ascent;
	clicker_text->font_descent = fontext.descent;
	clicker_text->font_height = fontext.height;
	clicker_text->font_max_x_advance = fontext.max_x_advance;
	clicker_text->font_max_y_advance = fontext.max_y_advance;

	cairo_select_font_face(clicker_text->cairo_ctx, DEFAULT_FONT,
			       CAIRO_FONT_SLANT_NORMAL,
			       CAIRO_FONT_WEIGHT_NORMAL);

	// cairo has ref count system, can call destroy on this
	cairo_surface_destroy(text_surface);
}

void text_free(struct clk_Text clicker_text)
{
	cairo_destroy(clicker_text.cairo_ctx);
}

void
text_debug_test(struct clk_Text clicker_text)
{
	cairo_font_extents_t fontext;

	cairo_set_font_size(clicker_text.cairo_ctx, 32.0);

	cairo_set_source_rgb(clicker_text.cairo_ctx, 1.0, 1.0, 1.0);

	cairo_move_to(clicker_text.cairo_ctx, 300.0, 100.0);

	cairo_font_extents(clicker_text.cairo_ctx, &fontext);

	cairo_show_text(clicker_text.cairo_ctx, "Hello, world!");

	cairo_move_to(clicker_text.cairo_ctx, 300.0,
		      fontext.height + fontext.descent);

	cairo_show_text(clicker_text.cairo_ctx, "Hello, world!");
}
