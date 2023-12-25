version = 1.0

srcdir = .

PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

ldlibs = $(LDLIBS)

all: release

.PHONY: all clean install uninstall clang debug release
.SUFFIXES:
.SUFFIXES: .c .o

CFLAGS += -std=c99 -D_DEFAULT_SOURCE
CFLAGS += -Wall -Wextra

clang: CFLAGS += -Weverything -Wno-unsafe-buffer-usage -Wno-format-nonliteral
clang: CC = clang
clang: clean release

debug: CFLAGS += -g
debug: clean bright

release: CFLAGS += -O2 -flto
release: bright

src = main.c send_signal.c
headers = bright.h send_signal.h

bright: $(src) $(headers) Makefile
	ctags --kinds-C=+l *.h *.c
	vtags.sed tags > .tags.vim
	$(CC) $(CFLAGS) $(LDFLAGS) -lm -o $@ $(src) $(ldlibs)

clean:
	rm -f *.o bright

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp bright $(DESTDIR)$(PREFIX)/bin/
	chmod 755 $(DESTDIR)$(PREFIX)/bin/bright
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	sed "s!PREFIX!$(PREFIX)!g; s!VERSION!$(version)!g" bright.1 >$(DESTDIR)$(MANPREFIX)/man1/bright.1
	chmod 644 $(DESTDIR)$(MANPREFIX)/man1/bright.1

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/bright
	rm -f $(DESTDIR)$(MANPREFIX)/man1/bright.1
