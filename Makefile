version = 1.0

srcdir = .

PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

ldlibs = $(LDLIBS)

objs = main.o send_signal.o

all: release

.PHONY: all clean install uninstall clang debug release
.SUFFIXES:
.SUFFIXES: .c .o

CFLAGS += -std=c99 -D_DEFAULT_SOURCE
CFLAGS += -Wall -Wextra

$(objs): Makefile bright.h send_signal.h

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

clang: CFLAGS += -Weverything -Wno-unsafe-buffer-usage
clang: CC = clang
clang: release

debug: CFLAGS += -g
debug: clean
debug: bright

release: CFLAGS += -O2
release: bright

bright: $(objs)
	ctags --kinds-C=+l *.h *.c
	vtags.sed tags > .tags.vim
	$(CC) $(CFLAGS) $(LDFLAGS) -lm -o $@ $(objs) $(ldlibs)

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
