#include "../src/internal.h"
#include <mr_utils.h>
#include <mr_utils/mrt_test.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct clk_Event clicker_event = { 0 };
struct clk_Renderer clicker_renderer = { 0 };
struct clk_EditorState clicker_state = { 0 };

global_variable long BUFFER_SIZE;

MRT_TEST_GROUP(create_destroy_buffer)
{
	BufferID b_id;
	BufferID b_id1;
	BufferID b_id2;

	Err err = buffer_create_blank(BUFFER_SIZE, &b_id);
	MRT_ASSERT(err == OK, "create buffer res OK");
	Buffer *buffer = BUFFERS_GET_BUFFER_BY_ID(b_id);
	MRT_ASSERT(buffer != NULL, "create buffer no file");
	buffer_destroy(b_id);

	buffer_create_blank(BUFFER_SIZE, &b_id);
	buffer_create_blank(BUFFER_SIZE, &b_id1);
	buffer_create_blank(BUFFER_SIZE, &b_id2);

	MRT_ASSERT(BUFFERS_GET_BUFFER_BY_ID(b_id) != NULL,
		   "create buffer 0 no file");
	MRT_ASSERT(BUFFERS_GET_BUFFER_BY_ID(b_id1) != NULL,
		   "create buffer 1 no file");
	MRT_ASSERT(BUFFERS_GET_BUFFER_BY_ID(b_id2) != NULL,
		   "create buffer 2 no file");

	buffer_destroy(b_id1);

	MRT_ASSERT(BUFFERS_GET_BUFFER_BY_ID(b_id) != NULL, "buffer 0 exists");
	MRT_ASSERT(BUFFERS_GET_BUFFER_BY_ID(b_id2) != NULL, "buffer 2 exists");
	MRT_ASSERT(BUFFERS_GET_BUFFER_BY_ID(b_id1) == NULL,
		   "buffer 1 destroyed");

	buffer_create_blank(BUFFER_SIZE, &b_id1);

	buffer_destroy(2);
	MRT_ASSERT(BUFFERS_GET_BUFFER_BY_ID(b_id2) == NULL,
		   "buffer 2 destroyed");
	MRT_ASSERT(BUFFERS_GET_BUFFER_BY_ID(b_id) != NULL, "buffer 0 exists");
	MRT_ASSERT(BUFFERS_GET_BUFFER_BY_ID(b_id1) != NULL, "buffer 1 exists");

	buffer_destroy(b_id);
	buffer_destroy(b_id1);

	const char *filestr =
		"this is a file\nnew lined here\ttab here\n\tnewline tab here\n\nnewline above here\n\n";

	FILE *file = fopen("./test/testdata/testfile.txt", "w+b");
	ASSERT(file != NULL);
	fputs(filestr, file);
	rewind(file);

	buffer_create_from_file(file, &b_id);
	buffer = BUFFERS_GET_BUFFER_BY_ID(b_id);

	MRT_ASSERT(strncmp(buffer->text + buffer->gap_end, filestr,
			   strlen(filestr)) == 0,
		   "filled buffer with file contents");

	buffer_destroy(b_id);
	fclose(file);
}

MRT_TEST_GROUP(move_buffer_gap)
{
	const char *filestr = "0123456789";

	FILE *file = fopen("./test/testdata/testfile1.txt", "w+b");
	ASSERT(file != NULL);
	fputs(filestr, file);
	rewind(file);

	BufferID b_id;
	buffer_create_from_file(file, &b_id);
	Buffer *buffer = BUFFERS_GET_BUFFER_BY_ID(b_id);

	// init state
	MRT_ASSERT(buffer->gap_start == 0, "init gap start 0");
	MRT_ASSERT(buffer->gap_end == BUFFER_MAX_TEXT_LENGTH(buffer->size) -
					      strlen(filestr),
		   "init gap end");

	// same gap position
	buffer_move_gap(b_id, 0);
	MRT_ASSERT(buffer->gap_start == 0, "move gap to 0 gap start");
	MRT_ASSERT(buffer->gap_end == BUFFER_MAX_TEXT_LENGTH(buffer->size) -
					      strlen(filestr),
		   "move gap to 0 gap end");

	// move gap right
	buffer_move_gap(b_id, 5);
	MRT_ASSERT(buffer->gap_start == 5, "move gap right to 5 start");
	MRT_ASSERT(buffer->gap_end == BUFFER_MAX_TEXT_LENGTH(buffer->size) -
					      strlen(filestr) + 5,
		   "move gap right to 5 end");
	MRT_ASSERT(strncmp(buffer->text, "01234", 5) == 0,
		   "text before gap strncmp");
	MRT_ASSERT(strncmp(buffer->text + buffer->gap_end, "56789", 5) == 0,
		   "text after gap strncmp");

	// move gap left
	buffer_move_gap(b_id, 2);
	MRT_ASSERT(buffer->gap_start == 2, "move gap left to 2 start");
	MRT_ASSERT(buffer->gap_end == BUFFER_MAX_TEXT_LENGTH(buffer->size) -
					      strlen(filestr) + 2,
		   "move gap left to 2 end");
	MRT_ASSERT(strncmp(buffer->text, "01", 2) == 0,
		   "text before gap strncmp");
	MRT_ASSERT(strncmp(buffer->text + buffer->gap_end, "23456789", 8) == 0,
		   "text after gap strncmp");

	// move gap to the end
	buffer_move_gap(b_id, strlen(filestr));
	MRT_ASSERT(buffer->gap_start == strlen(filestr),
		   "move gap to absolute end start");
	MRT_ASSERT(buffer->gap_end == BUFFER_MAX_TEXT_LENGTH(buffer->size),
		   "move gap to absolute end end");

	buffer_destroy(b_id);
	fclose(file);
}

MRT_TEST_GROUP(write_delete_char)
{
	BufferID b_id;
	buffer_create_blank(BUFFER_SIZE, &b_id);
	Buffer *buffer = BUFFERS_GET_BUFFER_BY_ID(b_id);

	buffer_insert_char(b_id, '0');
	buffer_insert_char(b_id, '1');
	buffer_insert_char(b_id, '2');
	buffer_insert_char(b_id, '3');
	buffer_insert_char(b_id, '4');
	buffer_insert_char(b_id, '5');
	buffer_insert_char(b_id, '6');
	buffer_insert_char(b_id, '7');
	buffer_insert_char(b_id, '8');
	buffer_insert_char(b_id, '9');

	// move gap right
	buffer_move_gap(b_id, 5);

	MRT_ASSERT(strncmp(buffer->text, "01234", 5) == 0,
		   "text before gap strncmp");
	MRT_ASSERT(strncmp(buffer->text + buffer->gap_end, "56789", 5) == 0,
		   "text after gap strncmp");

	buffer_insert_char(b_id, 'c');

	MRT_ASSERT(strncmp(buffer->text, "01234c", 6) == 0,
		   "text before gap strncmp after insert");
	MRT_ASSERT(strncmp(buffer->text + buffer->gap_end, "56789", 5) == 0,
		   "text after gap strncmp after insert");

	buffer_delete_char(b_id);

	MRT_ASSERT(strncmp(buffer->text, "01234", 5) == 0,
		   "text before gap strncmp");
	MRT_ASSERT(strncmp(buffer->text + buffer->gap_end, "56789", 5) == 0,
		   "text after gap strncmp");

	buffer_destroy(b_id);
}

MRT_TEST_GROUP(buffer_expand)
{
	BufferID b_id;
	buffer_create_blank(BUFFER_SIZE, &b_id);
	Buffer *buffer = BUFFERS_GET_BUFFER_BY_ID(b_id);
	size_t initial_size = buffer->size;

	buffer_expand_gap_by_page(b_id);
	buffer = BUFFERS_GET_BUFFER_BY_ID(b_id);

	MRT_ASSERT(buffer->size == initial_size + sysconf(_SC_PAGESIZE),
		   "buffer size increased by page size");

	buffer_insert_char(b_id, 'a');

	MRT_ASSERT(buffer->text[0] == 'a',
		   "buffer data integrity after expansion");

	// manually set full buffer
	buffer->gap_end = buffer->gap_start;
	// should cause a page allocation
	buffer_insert_char(b_id, 'a');

	buffer = BUFFERS_GET_BUFFER_BY_ID(b_id);
	MRT_ASSERT(
		buffer->size == initial_size + (2 * sysconf(_SC_PAGESIZE)),
		"buffer size increased by page size due to buffer_insert_char");

	buffer_destroy(b_id);
}

MRT_TEST_GROUP(debug_test)
{
	MRT_ASSERT(1 == 1, "sanity check");
}

int
main(void)
{
	MrlLogger *logger = mrl_create(stderr, FALSE, FALSE);
	MrtContext *ctx = mrt_ctx_create(logger);

	BUFFER_SIZE = sysconf(_SC_PAGESIZE);

	buffers_init();
	MRT_REGISTER_TEST_GROUP(ctx, debug_test);
	MRT_REGISTER_TEST_GROUP(ctx, create_destroy_buffer);
	MRT_REGISTER_TEST_GROUP(ctx, move_buffer_gap);
	MRT_REGISTER_TEST_GROUP(ctx, write_delete_char);
	MRT_REGISTER_TEST_GROUP(ctx, buffer_expand);

	Err err = mrt_ctx_run(ctx, FALSE);

	mrt_ctx_destroy(ctx);
	mrl_destroy(logger);

#ifdef DEBUG
	ASSERT(mrd_log_dump_active_allocations() == 0);
#endif

	return err;
}
