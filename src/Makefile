PROGRAM=mini-journalreader
SOURCES=mini-journalreader.c

BIN_DIR ?= $(DESTDIR)/usr/bin
MAN1_DIR ?= $(DESTDIR)/usr/share/man/man1

LIBS := libsystemd
CFLAGS += -Werror -Wall -Wextra -Wl,-z,relro -g -O2
CFLAGS += -fstack-protector-strong -D_FORTIFY_SOURCE=2
CFLAGS += $(shell pkg-config --cflags $(LIBS))
LDFLAGS += $(shell pkg-config --libs $(LIBS))

all: $(PROGRAM)

$(PROGRAM): $(SOURCES)
	gcc $< -o $@ $(CFLAGS) $(LDFLAGS)

.PHONY: install
install: $(PROGRAM) mini-journalreader.1
	install -d -m 0755 $(BIN_DIR)
	install -m 0755 $(PROGRAM) $(BIN_DIR)
	install -d -m 0755 $(MAN1_DIR)
	install -m 0644 mini-journalreader.1 $(MAN1_DIR)

mini-journalreader.1: mini-journalreader.scd
	scdoc < $< > $@

.PHONY: distclean
distclean: clean

.PHONY: clean
clean:
	rm -rf $(PROGRAM) mini-journalreader.1
