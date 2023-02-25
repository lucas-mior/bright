version = 1.0

srcdir = .

PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

cflags = -std=gnu99 -Wall -Wextra -pedantic $(CFLAGS)

ldlibs = $(LDLIBS)

objs = bright.o sig_dwmblocks.o

all: bright

.PHONY: all clean install uninstall
.SUFFIXES:
.SUFFIXES: .c .o

bear: Makefile
	bear -- make > compile_commands.json
$(objs): Makefile sig_dwmblocks.h

bright.o: sig_dwmblocks.h
sig_dwmblocks.o: sig_dwmblocks.h

.c.o:
	$(CC) -O2 $(cflags) $(cppflags) -lm -c -o $@ $<

bright: $(objs)
	$(CC) -O2 $(cflags) $(LDFLAGS) -lm -o $@ $(objs) $(ldlibs)
	ctags --kinds-C=+l *.h *.c
	vtags.sed tags > .tags.vim

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
	@echo "REMOVE bin/bright"
	rm -f $(DESTDIR)$(PREFIX)/bin/bright
	@echo "REMOVE bright.1"
	rm -f $(DESTDIR)$(MANPREFIX)/man1/bright.1
