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
TARGET    := $(BUILD_DIR)/cal

# find, not wildcard, so a new file is picked up at any depth. wildcard only
# matches the exact number of folder levels in the pattern, which silently
# ignores anything nested deeper.
SRCS := $(shell find src -name '*.c')
OBJS := $(SRCS:src/%.c=$(OBJ_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

.PHONY: all run clean install uninstall

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

# Object folders mirror src/ (build/obj/core/date.o), so two files with the
# same name in different domains can never overwrite each other. The mkdir
# lives in the recipe because the subfolders are not known up front.
$(OBJ_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)

# Installs as "cal". On most Linux and macOS systems a system cal already
# exists in /usr/bin, so make sure $(PREFIX)/bin comes first in PATH or
# change the installed name here before you rely on it.
install: $(TARGET)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/cal

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/cal

-include $(DEPS)