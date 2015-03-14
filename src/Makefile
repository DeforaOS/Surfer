SUBDIRS	= common
TARGETS	= $(OBJDIR)download $(OBJDIR)surfer
OBJDIR	=
PREFIX	= /usr/local
DESTDIR	=
BINDIR	= $(PREFIX)/bin
SBINDIR	= $(PREFIX)/sbin
CC	= cc
CPPFLAGSF=
CPPFLAGS=
CFLAGSF	= -W
CFLAGS	= -Wall -g -O2 -pedantic
LDFLAGS	= 
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: subdirs $(TARGETS)

subdirs:
	@for i in $(SUBDIRS); do (cd "$$i" && \
		if [ -n "$(OBJDIR)" ]; then \
		([ -d "$(OBJDIR)$$i" ] || $(MKDIR) -- "$(OBJDIR)$$i") && \
		$(MAKE) OBJDIR="$(OBJDIR)$$i/"; \
		else $(MAKE); fi) || exit; done

download_OBJS = $(OBJDIR)download.o
download_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) -D WITH_WEBKIT $(CFLAGSF) $(CFLAGS) `pkg-config --cflags libSystem gtk+-2.0 webkit-1.0`
download_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs libSystem gtk+-2.0 webkit-1.0` -lintl

$(OBJDIR)download: $(download_OBJS)
	$(CC) -o $(OBJDIR)download $(download_OBJS) $(download_LDFLAGS)

surfer_OBJS = $(OBJDIR)surfer.o $(OBJDIR)ghtml.o $(OBJDIR)callbacks.o $(OBJDIR)main.o
surfer_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) -D WITH_WEBKIT $(CFLAGSF) $(CFLAGS) `pkg-config --cflags libDesktop webkit-1.0`
surfer_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs libDesktop webkit-1.0` -lintl

$(OBJDIR)surfer: $(surfer_OBJS)
	$(CC) -o $(OBJDIR)surfer $(surfer_OBJS) $(surfer_LDFLAGS)

$(OBJDIR)download.o: download.c download.h ../config.h
	$(CC) -D WITH_MAIN $(download_CFLAGS) -o $(OBJDIR)download.o -c download.c

$(OBJDIR)surfer.o: surfer.c callbacks.h common/find.c download.c download.h surfer.h ../config.h
	$(CC) $(surfer_CFLAGS) -o $(OBJDIR)surfer.o -c surfer.c

$(OBJDIR)ghtml.o: ghtml.c ghtml.h ghtml-gtkhtml.c ghtml-gtkmozembed.c ghtml-gtktextview.c ghtml-webkit.c common/conn.c common/history.c common/url.c ../config.h
	$(CC) -D PREFIX=\"$(PREFIX)\" $(surfer_CFLAGS) -o $(OBJDIR)ghtml.o -c ghtml.c

$(OBJDIR)callbacks.o: callbacks.c surfer.h ghtml.h callbacks.h ../config.h
	$(CC) $(surfer_CFLAGS) -o $(OBJDIR)callbacks.o -c callbacks.c

$(OBJDIR)main.o: main.c surfer.h ../config.h
	$(CC) $(surfer_CFLAGS) -o $(OBJDIR)main.o -c main.c

clean:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) clean) || exit; done
	$(RM) -- $(download_OBJS) $(surfer_OBJS)

distclean:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) distclean) || exit; done
	$(RM) -- $(download_OBJS) $(surfer_OBJS)
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) install) || exit; done
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 $(OBJDIR)download $(DESTDIR)$(BINDIR)/download
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 $(OBJDIR)surfer $(DESTDIR)$(BINDIR)/surfer

uninstall:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) uninstall) || exit; done
	$(RM) -- $(DESTDIR)$(BINDIR)/download
	$(RM) -- $(DESTDIR)$(BINDIR)/surfer

.PHONY: all subdirs clean distclean install uninstall
