#include "../internal.h"
#include "./x11_internal.h"

void
draw_init(struct clk_Draw *clicker_draw, struct clk_Window clk_window)
{
	struct x11_Window *x11_window = clk_window.window_ctx;

	XWindowAttributes attributes;
	XGetWindowAttributes(x11_window->main_display, x11_window->main_window,
			     &attributes);

	clicker_draw->cairo_surface = cairo_xlib_surface_create(
		x11_window->main_display, x11_window->back_buffer,
		DefaultVisual(x11_window->main_display,
			      DefaultScreen(x11_window->main_display)),
		attributes.width, attributes.height);

	clicker_draw->cairo_ctx = cairo_create(clicker_draw->cairo_surface);

	cairo_select_font_face(clicker_draw->cairo_ctx, DEFAULT_FONT,
			       CAIRO_FONT_SLANT_NORMAL,
			       CAIRO_FONT_WEIGHT_NORMAL);

	cairo_font_extents_t fontext;
	cairo_font_extents(clicker_draw->cairo_ctx, &fontext);
	clicker_draw->current_font_ascent = fontext.ascent;
	clicker_draw->current_font_descent = fontext.descent;
	clicker_draw->current_font_height = fontext.height;
	clicker_draw->current_font_max_x_advance = fontext.max_x_advance;
	clicker_draw->current_font_max_y_advance = fontext.max_y_advance;
}

void
draw_update_text_surface_to_window_size(struct clk_Draw *clicker_draw,
					struct clk_Window clk_window)
{
	struct x11_Window *x11_window = clk_window.window_ctx;

	cairo_destroy(clicker_draw->cairo_ctx);
	cairo_surface_destroy(clicker_draw->cairo_surface);

	XFreePixmap(x11_window->main_display, x11_window->back_buffer);

	x11_window->back_buffer = XCreatePixmap(
		x11_window->main_display, x11_window->main_window,
		clk_window.window_w, clk_window.window_h,
		DefaultDepth(x11_window->main_display,
			     DefaultScreen(x11_window->main_display)));

	// recreate surface and context on new pixmap
	clicker_draw->cairo_surface = cairo_xlib_surface_create(
		x11_window->main_display, x11_window->back_buffer,
		DefaultVisual(x11_window->main_display,
			      DefaultScreen(x11_window->main_display)),
		clk_window.window_w, clk_window.window_h);

	clicker_draw->cairo_ctx = cairo_create(clicker_draw->cairo_surface);

	cairo_select_font_face(clicker_draw->cairo_ctx, DEFAULT_FONT,
			       CAIRO_FONT_SLANT_NORMAL,
			       CAIRO_FONT_WEIGHT_NORMAL);
}

void
draw_blit_present(struct clk_Window clk_window)
{
	struct x11_Window *x11 = clk_window.window_ctx;

	XCopyArea(x11->main_display, x11->back_buffer, x11->main_window,
		  x11->context, 0, 0, clk_window.window_w, clk_window.window_h,
		  0, 0);
}

void
draw_free(struct clk_Draw clicker_draw)
{
	cairo_destroy(clicker_draw.cairo_ctx);
	cairo_surface_destroy(clicker_draw.cairo_surface);
}

void
draw_push_attr(struct clk_Draw clicker_draw)
{
	cairo_save(clicker_draw.cairo_ctx);
}

void
draw_pop_attr(struct clk_Draw clicker_draw)
{
	cairo_restore(clicker_draw.cairo_ctx);
}

void
draw_set_font_size(struct clk_Draw clicker_draw, double size)
{
	cairo_set_font_size(clicker_draw.cairo_ctx, size);
}

void
draw_set_draw_color(struct clk_Draw clicker_draw, double r, double g, double b)
{
	cairo_set_source_rgb(clicker_draw.cairo_ctx, r, g, b);
}

void
draw_set_line_width(struct clk_Draw clicker_draw, uint16_t width)
{
	cairo_set_line_width(clicker_draw.cairo_ctx, width);
}

void
draw_move_cursor_to(struct clk_Draw clicker_draw, double x, double y)
{
	cairo_move_to(clicker_draw.cairo_ctx, x, y);
}

void
draw_relative_move_cursor_to(struct clk_Draw clicker_draw, double x, double y)
{
	cairo_rel_move_to(clicker_draw.cairo_ctx, x, y);
}

void
draw_update_font_extents(struct clk_Draw *clicker_draw)
{
	cairo_font_extents_t fontext;
	cairo_font_extents(clicker_draw->cairo_ctx, &fontext);

	clicker_draw->current_font_ascent = fontext.ascent;
	clicker_draw->current_font_descent = fontext.descent;
	clicker_draw->current_font_height = fontext.height;
	clicker_draw->current_font_max_x_advance = fontext.max_x_advance;
	clicker_draw->current_font_max_y_advance = fontext.max_y_advance;
}

void
draw_write_text(struct clk_Draw clicker_draw, const char *text,
		cairo_text_extents_t *text_extents)
{
	if (text_extents != NULL) {
		cairo_text_extents(clicker_draw.cairo_ctx, text, text_extents);
	}

	cairo_show_text(clicker_draw.cairo_ctx, text);
}

void
draw_flush(struct clk_Draw clicker_draw)
{
	cairo_surface_flush(clicker_draw.cairo_surface);
}

void
draw_line(struct clk_Draw clicker_draw, uint16_t x1, uint16_t y1, uint16_t x2,
	  uint16_t y2)
{
	draw_push_attr(clicker_draw);

	cairo_move_to(clicker_draw.cairo_ctx, x1, y1);
	cairo_line_to(clicker_draw.cairo_ctx, x2, y2);
	cairo_stroke(clicker_draw.cairo_ctx);

	draw_pop_attr(clicker_draw);
}

void
draw_fill_rectangle(struct clk_Draw clicker_draw, uint16_t x, uint16_t y,
		    uint16_t w, uint16_t h, float r, float g, float b,
		    cairo_operator_t operator)
{
	draw_push_attr(clicker_draw);
	cairo_set_source_rgb(clicker_draw.cairo_ctx, r, g, b);

	cairo_set_operator(clicker_draw.cairo_ctx, operator);

	cairo_rectangle(clicker_draw.cairo_ctx, x, y, w, h);
	cairo_fill(clicker_draw.cairo_ctx);

	draw_pop_attr(clicker_draw);
}

void
draw_clip_rectangle(struct clk_Draw clicker_draw, double x, double y, double w,
		    double h)
{
	cairo_rectangle(clicker_draw.cairo_ctx, x, y, w, h);

	cairo_clip(clicker_draw.cairo_ctx);
}

void
draw_color_background(struct clk_Draw clicker_draw)
{
	cairo_paint(clicker_draw.cairo_ctx);
}
