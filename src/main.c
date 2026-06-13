#include "utils.h"
#include "window.h"
#include <mr_utils.h>
#include <unistd.h>

int
main(int argc, char **argv)
{
	window_init();

	clicker_Window *window = window_create(0, 0, 800, 600, 0);

	struct clicker_WindowEvent event;
	for (;;) {
		window_get_event(window, &event);
		if (event.type == CLICKER_WINDOW_EVENT_TYPE_CLOSEREQ) {
			break;
		}

		window_clear(window);
#ifdef DEBUG
		char debug_event_snack_text[128];
		sprintf(debug_event_snack_text,
			"eventtype: %d \nkeybutton_val: %d \nmouse_x: %d \nmouse_y: %d",
			event.type, event.val.keycode, event.val.mouse.x,
			event.val.mouse.y);
		window_draw_debug_snack(window, debug_event_snack_text);
#endif

		window_flush_display();
	}

	window_destroy(window);
	window_cleanup();

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

	// TODO: remove later when impl splash screen
	if (argc == 1) {
		log_help();
	}

	// TODO the actual things

	return 0;
}
