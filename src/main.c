#include "./internal.h"
#include <mr_utils.h>
#include <unistd.h>

int
main(int argc, char **argv)
{
	Err err = OK;
	for (size_t i = 1; i < (size_t)argc; i++) {
		switch (argv[i][0]) {
		case '-':
			err = process_arg(argv[i]);
			break;
		default:
			// TODO validate file names/ paths
			break;
		}

		if (err == ERR) {
			return err;
		}
	}

	buffers_init();
	clk_Window *clicker_window = window_create(0, 0, 800, 600, 0);

	struct clk_WindowEvent clicker_event = { 0 };

	for (;;) {
		window_get_event(clicker_window, &clicker_event);
		if (clicker_event.type == CLK_WINDOW_EVENT_TYPE_CLOSEREQ) {
			break;
		}

		window_clear(clicker_window);
#ifdef DEBUG
		char debug_event_snack_text[128];

		sprintf(debug_event_snack_text,
			"eventtype: %d \nkeybutton_val: %d \nmouse_x: %d \nmouse_y: %d",
			clicker_event.type, clicker_event.val.keycode,
			clicker_event.val.mouse.x, clicker_event.val.mouse.y);

		window_draw_debug_snack(clicker_window, debug_event_snack_text);
#endif

		window_flush_display(clicker_window);
	}

	window_destroy(clicker_window);

	return 0;
}
