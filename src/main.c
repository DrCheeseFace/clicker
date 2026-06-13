#include "utils.h"
#include "window.h"
#include <mr_utils.h>
#include <unistd.h>

int
main(int argc, char **argv)
{
	window_init();

	clicker_Window *window = window_create(0, 0, 800, 600, 0);
	sleep(5);
	window_destroy(window);

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
