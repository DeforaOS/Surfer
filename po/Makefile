TARGETS	= $(OBJDIR)Surfer.pot $(OBJDIR)de.mo $(OBJDIR)es.mo $(OBJDIR)fr.mo $(OBJDIR)it.mo
OBJDIR	=
PREFIX	= /usr/local
DESTDIR	=
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

$(OBJDIR)Surfer.pot: POTFILES
	./gettext.sh -P "$(PREFIX)" -- "$(OBJDIR)Surfer.pot"

$(OBJDIR)de.mo: $(OBJDIR)Surfer.pot de.po
	./gettext.sh -P "$(PREFIX)" -- "$(OBJDIR)de.mo"

$(OBJDIR)es.mo: $(OBJDIR)Surfer.pot es.po
	./gettext.sh -P "$(PREFIX)" -- "$(OBJDIR)es.mo"

$(OBJDIR)fr.mo: $(OBJDIR)Surfer.pot fr.po
	./gettext.sh -P "$(PREFIX)" -- "$(OBJDIR)fr.mo"

$(OBJDIR)it.mo: $(OBJDIR)Surfer.pot it.po
	./gettext.sh -P "$(PREFIX)" -- "$(OBJDIR)it.mo"

clean:
	$(RM) -- $(Surfer.pot_OBJS) $(de.mo_OBJS) $(es.mo_OBJS) $(fr.mo_OBJS) $(it.mo_OBJS)
	./gettext.sh -c -P "$(PREFIX)" -- "Surfer.pot"
	./gettext.sh -c -P "$(PREFIX)" -- "de.mo"
	./gettext.sh -c -P "$(PREFIX)" -- "es.mo"
	./gettext.sh -c -P "$(PREFIX)" -- "fr.mo"
	./gettext.sh -c -P "$(PREFIX)" -- "it.mo"

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	./gettext.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)de.mo"
	./gettext.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)es.mo"
	./gettext.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)fr.mo"
	./gettext.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)it.mo"

uninstall:
	./gettext.sh -P "$(DESTDIR)$(PREFIX)" -u -- "de.mo"
	./gettext.sh -P "$(DESTDIR)$(PREFIX)" -u -- "es.mo"
	./gettext.sh -P "$(DESTDIR)$(PREFIX)" -u -- "fr.mo"
	./gettext.sh -P "$(DESTDIR)$(PREFIX)" -u -- "it.mo"

.PHONY: all clean distclean install uninstall
