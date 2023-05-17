PROGRAM=mini-journalreader
SOURCES=mini-journalreader.c

BIN_DIR ?= $(DESTDIR)/usr/bin

LIBS := libsystemd
CFLAGS += -Werror -Wall -Wextra -Wl,-z,relro -g -O2
CFLAGS += -fstack-protector-strong -D_FORTIFY_SOURCE=2
CFLAGS += $(shell pkg-config --cflags $(LIBS))
LDFLAGS += $(shell pkg-config --libs $(LIBS))

all: $(PROGRAM)

$(PROGRAM): $(SOURCES)
	gcc $< -o $@ $(CFLAGS) $(LDFLAGS)

.PHONY: install
install: $(PROGRAM) proxmox-mini-journalreader.1
	install -d -m 0755 $(BIN_DIR)
	install -m 0755 $(PROGRAM) $(BIN_DIR)

.PHONY: distclean
distclean: clean

.PHONY: clean
clean:
	rm -rf $(PROGRAM)
