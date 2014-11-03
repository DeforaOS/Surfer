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
LDFLAGS	= 
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

helper_OBJS = $(OBJDIR)helper.o
helper_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
helper_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs libDesktop webkit-1.0` ../src/ghtml.o

$(OBJDIR)helper: $(helper_OBJS) ../src/ghtml.o
	$(CC) -o $(OBJDIR)helper $(helper_OBJS) $(helper_LDFLAGS)

$(OBJDIR)helper.o: helper.c ../src/common/find.c ../src/ghtml.c ../src/surfer.h ../config.h
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
