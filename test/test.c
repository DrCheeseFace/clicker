#include "../src/internal.h"
#include <mr_utils.h>
#include <mr_utils/mrt_test.h>
#include <stdlib.h>
#include <string.h>

MRT_TEST_GROUP(create_destroy_buffer)
{
	Buffer *buffer = buffer_create(NULL);
	MRT_ASSERT(buffer != NULL, "create buffer no file");
	buffer_destroy(buffer);

	buffer = buffer_create(NULL);
	Buffer *buffer1 = buffer_create(NULL);
	Buffer *buffer2 = buffer_create(NULL);

	MRT_ASSERT(buffer != NULL, "create buffer no file");
	MRT_ASSERT(buffer1 != NULL, "create buffer1 no file");
	MRT_ASSERT(buffer2 != NULL, "create buffer2 no file");
	MRT_ASSERT(buffer_count == 3, "buffer_count check");

	buffer_destroy(buffer1);

	MRT_ASSERT(buffer != NULL, "buffer exists");
	MRT_ASSERT(buffer2 != NULL, "buffer2 exists");
	MRT_ASSERT(buffer_count == 2, "buffer_count check");
	MRT_ASSERT(buffers[1] == buffer2, "delete and replace from end");
	MRT_ASSERT(buffers[2] == NULL, "moved buffer pointer set to null");

	buffer1 = buffer_create(NULL);

	buffer_destroy(buffer2);
	MRT_ASSERT(buffers[2] == NULL, "moved buffer pointer set to null");
	MRT_ASSERT(buffers[0] == buffer, "buffer exists");
	MRT_ASSERT(buffers[1] == buffer1, "buffer1 exists");

	buffer_destroy(buffer);
	buffer_destroy(buffer1);

	const char *filestr =
		"this is a file\nnew lined here\ttab here\n\tnewline tab here\n\nnewline above here\n\n";

	FILE *file = fopen("./test/testdata/testfile.txt", "w+b");
	ASSERT(file != NULL);
	fputs(filestr, file);
	rewind(file);

	buffer = buffer_create(file);

	MRT_ASSERT(strncmp(buffer->text + buffer->gap_end, filestr,
			   strlen(filestr)) == 0,
		   "filled buffer with file contents");

	buffer_destroy(buffer);
	fclose(file);
}

MRT_TEST_GROUP(move_buffer_gap)
{
	const char *filestr = "0123456789";

	FILE *file = fopen("./test/testdata/testfile1.txt", "w+b");
	ASSERT(file != NULL);
	fputs(filestr, file);
	rewind(file);

	Buffer *buffer = buffer_create(file);
	memcpy(buffer->text + buffer->gap_end, filestr, strlen(filestr));

	// init state
	MRT_ASSERT(buffer->gap_start == 0, "init gap start 0");
	MRT_ASSERT(buffer->gap_end == MAX_BUFFER_TEXT_LEN - strlen(filestr),
		   "init gap end");

	// same gap position
	buffer_move_gap(buffer, 0);
	MRT_ASSERT(buffer->gap_start == 0, "move gap to 0 gap start");
	MRT_ASSERT(buffer->gap_end == MAX_BUFFER_TEXT_LEN - strlen(filestr),
		   "move gap to 0 gap end");

	// move gap right
	buffer_move_gap(buffer, 5);
	MRT_ASSERT(buffer->gap_start == 5, "move gap right to 5 start");
	MRT_ASSERT(buffer->gap_end == MAX_BUFFER_TEXT_LEN - strlen(filestr) + 5,
		   "move gap right to 5 end");
	MRT_ASSERT(strncmp(buffer->text, "01234", 5) == 0,
		   "text before gap strncmp");
	MRT_ASSERT(strncmp(buffer->text + buffer->gap_end, "56789", 5) == 0,
		   "text after gap strncmp");

	// move gap left
	buffer_move_gap(buffer, 2);
	MRT_ASSERT(buffer->gap_start == 2, "move gap left to 2 start");
	MRT_ASSERT(buffer->gap_end == MAX_BUFFER_TEXT_LEN - strlen(filestr) + 2,
		   "move gap left to 2 end");
	MRT_ASSERT(strncmp(buffer->text, "01", 2) == 0,
		   "text before gap strncmp");
	MRT_ASSERT(strncmp(buffer->text + buffer->gap_end, "23456789", 8) == 0,
		   "text after gap strncmp");

	// move gap to the end
	buffer_move_gap(buffer, strlen(filestr));
	MRT_ASSERT(buffer->gap_start == strlen(filestr),
		   "move gap to absolute end start");
	MRT_ASSERT(buffer->gap_end == MAX_BUFFER_TEXT_LEN,
		   "move gap to absolute end end");

	buffer_destroy(buffer);
	fclose(file);
}

MRT_TEST_GROUP(debug_test)
{
	MRT_ASSERT(1 == 1, "sanity check");
}

int
main(void)
{
	MrlLogger *logger = mrl_create(stderr, TRUE, FALSE);
	MrtContext *ctx = mrt_ctx_create(logger);

	MRT_REGISTER_TEST_GROUP(ctx, debug_test);
	MRT_REGISTER_TEST_GROUP(ctx, create_destroy_buffer);
	MRT_REGISTER_TEST_GROUP(ctx, move_buffer_gap);

#ifdef DEBUG
	Err err = mrt_ctx_run(ctx, FALSE);
#else
	Err err = mrt_ctx_run(ctx, TRUE);
#endif

	mrt_ctx_destroy(ctx);
	mrl_destroy(logger);

#ifdef DEBUG
	ASSERT(mrd_log_dump_active_allocations() == 0);
#endif

	return err;
}
