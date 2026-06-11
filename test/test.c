#include "../src/buffers.h"
#include <mr_utils.h>
#include <mr_utils/mrt_test.h>

MRT_TEST_GROUP(create_destroy_buffer)
{
	Buffer *buffer = buffer_create(NULL);
	MRT_ASSERT(buffer != NULL, "create buffer no file");
	buffer_destory(buffer);

	buffer = buffer_create(NULL);
	Buffer *buffer1 = buffer_create(NULL);
	Buffer *buffer2 = buffer_create(NULL);

	MRT_ASSERT(buffer != NULL, "create buffer no file");
	MRT_ASSERT(buffer1 != NULL, "create buffer1 no file");
	MRT_ASSERT(buffer2 != NULL, "create buffer2 no file");
	MRT_ASSERT(buffer_count == 3, "buffer_count check");

	buffer_destory(buffer1);

	MRT_ASSERT(buffer != NULL, "buffer exists");
	MRT_ASSERT(buffer2 != NULL, "buffer2 exists");
	MRT_ASSERT(buffer_count == 2, "buffer_count check");
	MRT_ASSERT(buffers[1] == buffer2, "delete and replace from end");
	MRT_ASSERT(buffers[2] == NULL, "moved buffer pointer set to null");

	buffer1 = buffer_create(NULL);

	buffer_destory(buffer2);
	MRT_ASSERT(buffers[2] == NULL, "moved buffer pointer set to null");
	MRT_ASSERT(buffers[0] == buffer, "buffer exists");
	MRT_ASSERT(buffers[1] == buffer1, "buffer1 exists");

	buffer_destory(buffer);
	buffer_destory(buffer1);
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
