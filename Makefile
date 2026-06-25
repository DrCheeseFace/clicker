CC          = gcc
CSTANDARD   = c99

INCLUDES    = -Iinclude -Isrc/mr_utils/include
LDLIBS      = -lm -lX11
# LDLIBS      += -lasan

WARNINGS  = -Wall -Wextra -Werror -Wpedantic -pedantic-errors
WARNINGS += -Wpointer-arith -Wcast-align -Wwrite-strings
WARNINGS += -Wstrict-prototypes
WARNINGS += -Wswitch-default -Wunreachable-code
WARNINGS += -Wbad-function-cast -Wcast-qual -Wundef
WARNINGS += -Wshadow -Wfloat-equal -Wformat=2
WARNINGS += -Wredundant-decls -Wnested-externs
# WARNINGS += -fsanitize=address

BUILD_TYPE ?= release

ifneq (,$(filter debug build-debug build-test-debug test-debug,$(MAKECMDGOALS)))
    BUILD_TYPE := debug
endif

# -DMRD_DEBUG_DEFAULT
# -DMRD_DEBUG_ONLY_CALLED_AND_ERR
ifeq ($(BUILD_TYPE),debug)
    CFLAGS     := -O0 -g -fno-omit-frame-pointer -rdynamic -DDEBUG -DMRD_DEBUG_DEFAULT $(WARNINGS) $(INCLUDES)
else
    CFLAGS     := -O2 $(WARNINGS) $(INCLUDES)
endif

BUILD_DIR := build
OBJ_DIR   := $(BUILD_DIR)/$(BUILD_TYPE)

TARGET_MAIN    = $(OBJ_DIR)/main.out
TARGET_TEST    = $(OBJ_DIR)/test.out
TARGET_SPACERS = $(OBJ_DIR)/spacers

# DO BETTER LOL
SRC_LIB        = src/main.c src/posix/buffers.c src/utils.c src/x11/window.c src/render.c src/editor.c
SRC_TEST_MAIN  = test/test.c src/posix/buffers.c src/utils.c src/x11/window.c src/render.c src/editor.c

SRC_MR_UTILS   = src/mr_utils/src/mrd_debug.c \
                 src/mr_utils/src/mrl_logger.c \
                 src/mr_utils/src/mrs_strings.c \
                 src/mr_utils/src/mrt_test.c \
                 src/mr_utils/src/mrv_vectors.c

SRC_SPACERS    = src/mr_utils/tools/spacers.c

OBJ_LIB        = $(SRC_LIB:%.c=$(OBJ_DIR)/%.o)
OBJ_TEST_MAIN  = $(SRC_TEST_MAIN:%.c=$(OBJ_DIR)/%.o)
OBJ_MR_UTILS   = $(SRC_MR_UTILS:%.c=$(OBJ_DIR)/%.o)
OBJ_SPACERS    = $(SRC_SPACERS:%.c=$(OBJ_DIR)/%.o)

ALL_MAIN_OBJS    = $(OBJ_LIB) $(OBJ_MR_UTILS)
ALL_TEST_OBJS    = $(OBJ_TEST_MAIN) $(OBJ_MR_UTILS)
ALL_SPACERS_OBJS = $(OBJ_MR_UTILS) $(OBJ_SPACERS)

.PHONY: all test run clean format format-check debug build-debug spacers tags

all: $(TARGET_TEST) $(TARGET_SPACERS)

$(TARGET_TEST): $(ALL_TEST_OBJS)
	$(CC) $(ALL_TEST_OBJS) -o $@ $(LDLIBS)

$(TARGET_MAIN): $(ALL_MAIN_OBJS)
	$(CC) $(ALL_MAIN_OBJS) -o $@ $(LDLIBS)

$(TARGET_SPACERS): $(ALL_SPACERS_OBJS)
	$(CC) $(ALL_SPACERS_OBJS) -o $@ $(LDLIBS)

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -MD -c $< -o $@ -std=$(CSTANDARD) $(CFLAGS)

test: $(TARGET_TEST)
	./$(TARGET_TEST)

run: $(TARGET_MAIN)
	./$(TARGET_MAIN)

build-debug: tags $(TARGET_MAIN) $(TARGET_SPACERS)

build-test-debug: tags $(TARGET_TEST) $(TARGET_SPACERS)

debug: build-debug
	./$(TARGET_MAIN)

test-debug: build-test-debug
	./$(TARGET_TEST)

clean:
	rm -rf $(BUILD_DIR)
	rm -f cscope.*
	rm -f tags

tags:
	rm -f cscope.*
	rm -f tags
	find ./ -name '*.c' -o -name '*.h' > ./cscope.files
	echo /usr/include/string.h >> ./cscope.files
	echo /usr/include/stdio.h >> ./cscope.files
	echo /usr/include/stdlib.h >> ./cscope.files
	echo /usr/include/unistd.h >> ./cscope.files
	echo /usr/include/math.h >> ./cscope.files
	echo /usr/include/sys/mman.h >> ./cscope.files
	find /usr/include/X11 -type f -name '*.[ch]' 2>/dev/null >> ./cscope.files
	cscope -b -q -k
	ctags-universal -L ./cscope.files -f ./tags --extras=+q --fields=+n --extras=+p --kinds-c=+p

spacers: $(TARGET_SPACERS)

format: $(TARGET_SPACERS)
	find ./src ./test -name "*.c" -o -name "*.h" | xargs clang-format -i --verbose
	git ls-files --recurse-submodules | xargs $(TARGET_SPACERS)

format-check: $(TARGET_SPACERS)
	find ./src ./test -name "*.c" -o -name "*.h" | xargs clang-format --dry-run --Werror --verbose
	git ls-files --recurse-submodules | xargs $(TARGET_SPACERS)

valgrind:
	valgrind --leak-check=full --suppressions=valgrind.supp $(TARGET_TEST)

record:
	perf record -g --call-graph dwarf $(TARGET_TEST)
	perf script > chombo.perf

-include $(ALL_TEST_OBJS:.o=.d)
-include $(OBJ_SPACERS:.o=.d)
