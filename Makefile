# -pedantic plus -std=c11 keeps us honest about portability. The only POSIX
# bits (isatty, strncasecmp) are opted into inside cal.c via the feature
# macro, so the strict flags here do not fight them.
CFLAGS  ?= -std=c11 -Wall -Wextra -pedantic -O2

PREFIX  ?= /usr/local
BIN_DIR := build
TARGET  := $(BIN_DIR)/cal
SRC     := src/cal.c

.PHONY: all run clean install uninstall

all: $(TARGET)

# build/ is created as an order-only prerequisite (after the pipe) so a
# timestamp change on the directory never triggers a pointless rebuild.
$(TARGET): $(SRC) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $<

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BIN_DIR)

# Installs as "cal". On most Linux and macOS systems a system cal already
# exists in /usr/bin, so make sure $(PREFIX)/bin comes first in PATH or
# change the installed name here before you rely on it.
install: $(TARGET)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/cal

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/cal