TARGETS	= $(OBJDIR)download.1 $(OBJDIR)download.html $(OBJDIR)helper.1 $(OBJDIR)helper.html $(OBJDIR)surfer.1 $(OBJDIR)surfer.html
OBJDIR	=
PREFIX	= /usr/local
DESTDIR	=
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

$(OBJDIR)download.1: download.xml
	./docbook.sh -P "$(PREFIX)" -- "$(OBJDIR)download.1"

$(OBJDIR)download.html: download.css.xml download.xml manual.css.xml
	./docbook.sh -P "$(PREFIX)" -- "$(OBJDIR)download.html"

$(OBJDIR)helper.1: helper.xml
	./docbook.sh -P "$(PREFIX)" -- "$(OBJDIR)helper.1"

$(OBJDIR)helper.html: helper.css.xml helper.xml manual.css.xml
	./docbook.sh -P "$(PREFIX)" -- "$(OBJDIR)helper.html"

$(OBJDIR)surfer.1: surfer.xml
	./docbook.sh -P "$(PREFIX)" -- "$(OBJDIR)surfer.1"

$(OBJDIR)surfer.html: manual.css.xml surfer.css.xml surfer.xml
	./docbook.sh -P "$(PREFIX)" -- "$(OBJDIR)surfer.html"

clean:
	$(RM) -- $(download.1_OBJS) $(download.html_OBJS) $(helper.1_OBJS) $(helper.html_OBJS) $(surfer.1_OBJS) $(surfer.html_OBJS)
	./docbook.sh -c -P "$(PREFIX)" -- "download.1"
	./docbook.sh -c -P "$(PREFIX)" -- "download.html"
	./docbook.sh -c -P "$(PREFIX)" -- "helper.1"
	./docbook.sh -c -P "$(PREFIX)" -- "helper.html"
	./docbook.sh -c -P "$(PREFIX)" -- "surfer.1"
	./docbook.sh -c -P "$(PREFIX)" -- "surfer.html"

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)download.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)download.html"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)helper.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)helper.html"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)surfer.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)surfer.html"

uninstall:
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "download.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "download.html"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "helper.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "helper.html"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "surfer.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "surfer.html"

.PHONY: all clean distclean install uninstall
