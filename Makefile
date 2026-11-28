# -pedantic plus -std=c11 keeps us honest about portability. The only POSIX
# bits (isatty, strncasecmp) are opted into inside the cli files via the
# feature macro, so the strict flags here do not fight them.
CFLAGS    ?= -std=c11 -Wall -Wextra -pedantic -O2

# -Isrc means every include is written from the src root ("core/date.h"), so
# moving a file between folders never means rewriting relative "../" paths.
# -MMD -MP makes the compiler write a .d file per object listing the headers
# it read. Without that, editing a .h would not rebuild the .c files using it
# and you would be debugging a stale binary.
CPPFLAGS  += -Isrc -MMD -MP

PREFIX    ?= /usr/local
BUILD_DIR := build
OBJ_DIR   := $(BUILD_DIR)/obj
TEST_DIR  := $(BUILD_DIR)/tests
TARGET    := $(BUILD_DIR)/mycal

# find, not wildcard, so a new file is picked up at any depth. wildcard only
# matches the exact number of folder levels in the pattern, which silently
# ignores anything nested deeper.
SRCS := $(shell find src -name '*.c')
OBJS := $(SRCS:src/%.c=$(OBJ_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

# Every tests/**/test_*.c is its own program with its own main(). They link
# against all the objects except main.o, which would clash with theirs.
TEST_SRCS := $(shell find tests -name 'test_*.c')
TEST_BINS := $(TEST_SRCS:tests/%.c=$(TEST_DIR)/%)
LIB_OBJS  := $(filter-out $(OBJ_DIR)/main.o,$(OBJS))

.PHONY: all run test sanitize clean install uninstall

all: $(TARGET)

# CFLAGS goes on the link line too. Flags like -fsanitize=address change what
# the linker has to pull in, and without them the link fails with hundreds of
# undefined ___asan_* symbols.
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

# Object folders mirror src/ (build/obj/core/calendar/calendar.o), so two
# files with the same name in different domains can never overwrite each
# other. The mkdir lives in the recipe because the subfolders are not known
# up front.
$(OBJ_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

run: $(TARGET)
	./$(TARGET)

# -Itests lets test files write #include "support/test.h". Each binary depends
# on the library objects, so a changed source or header recompiles the test.
$(TEST_DIR)/%: tests/%.c $(LIB_OBJS)
	@mkdir -p $(dir $@)
	$(CC) -Isrc -Itests $(CFLAGS) -o $@ $< $(LIB_OBJS)

# Runs every test binary even after a failure, so one broken module does not
# hide the results of the others, then fails the build if any of them failed.
# The smoke test runs the real binary end to end, which covers main.c (command
# dispatch, exit codes) that the unit tests are not linked against.
test: $(TEST_BINS) $(TARGET)
	@failed=0; \
	for t in $(TEST_BINS); do ./$$t || failed=1; done; \
	sh tests/smoke/smoke.sh $(TARGET) || failed=1; \
	exit $$failed

# The same tests built with AddressSanitizer and UndefinedBehaviorSanitizer,
# in their own folder. Never build sanitizer objects into build/ itself: make
# only compares file times, not flags, so a later plain "make" would reuse
# them and fail to link.
SAN_FLAGS := -std=c11 -Wall -Wextra -pedantic -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer

sanitize:
	$(MAKE) BUILD_DIR=$(BUILD_DIR)/sanitize CFLAGS="$(SAN_FLAGS)" test

clean:
	rm -rf $(BUILD_DIR)

# Installed as "mycal", not "cal": every Linux and macOS system already has a
# cal in /usr/bin, and a program that silently shadows it (or loses to it in
# PATH) is a confusing thing to debug. Override with BIN_NAME=... if you like.
BIN_NAME ?= mycal

install: $(TARGET)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(BIN_NAME)

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(BIN_NAME)

-include $(DEPS)