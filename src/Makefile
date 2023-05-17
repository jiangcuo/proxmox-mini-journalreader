PROGRAM=mini-journalreader
SOURCES=mini-journalreader.c

LIBS := libsystemd
CFLAGS += -Werror -Wall -Wextra -Wl,-z,relro -g -O2
CFLAGS += -fstack-protector-strong -D_FORTIFY_SOURCE=2
CFLAGS += $(shell pkg-config --cflags $(LIBS))
LFLAGS += $(shell pkg-config --libs $(LIBS))

all: $(PROGRAM)

$(PROGRAM): $(SOURCES)
	gcc $< -o $@ $(CFLAGS) $(LFLAGS)

.PHONY: install
install: $(PROGRAM)
	mkdir -p $(DESTDIR)/usr/bin
	install -m 0755 $(PROGRAM) $(DESTDIR)/usr/bin

.PHONY: distclean
distclean: clean

.PHONY: clean
clean:
	rm -rf $(PROGRAM)
