TARGETS	= $(OBJDIR)helper
OBJDIR	=
PREFIX	= /usr/local
DESTDIR	=
BINDIR	= $(PREFIX)/bin
SBINDIR	= $(PREFIX)/sbin
CC	= cc
CPPFLAGSF= -I ../src
CPPFLAGS=
CFLAGSF	= -W `pkg-config --cflags libDesktop`
CFLAGS	= -Wall -g -O2 -pedantic
LDFLAGS	= `pkg-config --libs libDesktop`
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

helper_OBJS = $(OBJDIR)ghtml-helper.o $(OBJDIR)helper.o
helper_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags webkit-1.0`
helper_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs webkit-1.0`

$(OBJDIR)helper: $(helper_OBJS)
	$(CC) -o $(OBJDIR)helper $(helper_OBJS) $(helper_LDFLAGS)

$(OBJDIR)ghtml-helper.o: ghtml-helper.c ../src/ghtml.h ../src/ghtml.c
	$(CC) -D WITH_WEBKIT $(helper_CFLAGS) -o $(OBJDIR)ghtml-helper.o -c ghtml-helper.c

$(OBJDIR)helper.o: helper.c ../src/common/find.c ../src/surfer.h ../config.h
	$(CC) -D WITH_WEBKIT $(helper_CFLAGS) -o $(OBJDIR)helper.o -c helper.c

clean:
	$(RM) -- $(helper_OBJS)

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 $(OBJDIR)helper $(DESTDIR)$(BINDIR)/helper

uninstall:
	$(RM) -- $(DESTDIR)$(BINDIR)/helper

.PHONY: all clean distclean install uninstall
