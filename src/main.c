#include "./internal.h"
#include <mr_utils.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

struct clk_Event clicker_event = { 0 };
struct clk_Renderer clicker_renderer = { 0 };
struct clk_EditorState clicker_state = { 0 };

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

	render_init(&clicker_renderer, 0, 0, 800, 600, 0);
	editor_init(&clicker_state, NULL);

	while (clicker_state.is_running) {
		window_pol_event();

		editor_simulate(&clicker_state, clicker_event);

		render_frame(&clicker_renderer, &clicker_state);
	}

	editor_free(&clicker_state);
	render_free(&clicker_renderer);

#ifdef DEBUG
	cairo_debug_reset_static_data();

	ASSERT(mrd_log_dump_active_allocations() == 0);
#endif
	return 0;
}
