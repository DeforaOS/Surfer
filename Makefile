PACKAGE	= Surfer
VERSION	= 0.2.6
SUBDIRS	= data doc po src tools
RM	= rm -f
LN	= ln -f
TAR	= tar
MKDIR	= mkdir -m 0755 -p


all: subdirs

subdirs:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE)) || exit; done

clean:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) clean) || exit; done

distclean:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) distclean) || exit; done

dist:
	$(RM) -r -- $(PACKAGE)-$(VERSION)
	$(LN) -s -- . $(PACKAGE)-$(VERSION)
	@$(TAR) -czvf $(PACKAGE)-$(VERSION).tar.gz -- \
		$(PACKAGE)-$(VERSION)/data/Makefile \
		$(PACKAGE)-$(VERSION)/data/deforaos-helper.desktop \
		$(PACKAGE)-$(VERSION)/data/deforaos-surfer.desktop \
		$(PACKAGE)-$(VERSION)/data/project.conf \
		$(PACKAGE)-$(VERSION)/data/16x16/Makefile \
		$(PACKAGE)-$(VERSION)/data/16x16/surfer-view-html-source.png \
		$(PACKAGE)-$(VERSION)/data/16x16/project.conf \
		$(PACKAGE)-$(VERSION)/data/24x24/Makefile \
		$(PACKAGE)-$(VERSION)/data/24x24/surfer-view-html-source.png \
		$(PACKAGE)-$(VERSION)/data/24x24/project.conf \
		$(PACKAGE)-$(VERSION)/doc/Makefile \
		$(PACKAGE)-$(VERSION)/doc/docbook.sh \
		$(PACKAGE)-$(VERSION)/doc/download.css.xml \
		$(PACKAGE)-$(VERSION)/doc/download.xml \
		$(PACKAGE)-$(VERSION)/doc/helper.css.xml \
		$(PACKAGE)-$(VERSION)/doc/helper.xml \
		$(PACKAGE)-$(VERSION)/doc/manual.css.xml \
		$(PACKAGE)-$(VERSION)/doc/surfer.css.xml \
		$(PACKAGE)-$(VERSION)/doc/surfer.xml \
		$(PACKAGE)-$(VERSION)/doc/project.conf \
		$(PACKAGE)-$(VERSION)/po/Makefile \
		$(PACKAGE)-$(VERSION)/po/gettext.sh \
		$(PACKAGE)-$(VERSION)/po/POTFILES \
		$(PACKAGE)-$(VERSION)/po/de.po \
		$(PACKAGE)-$(VERSION)/po/es.po \
		$(PACKAGE)-$(VERSION)/po/fr.po \
		$(PACKAGE)-$(VERSION)/po/it.po \
		$(PACKAGE)-$(VERSION)/po/project.conf \
		$(PACKAGE)-$(VERSION)/src/download.c \
		$(PACKAGE)-$(VERSION)/src/surfer.c \
		$(PACKAGE)-$(VERSION)/src/ghtml.c \
		$(PACKAGE)-$(VERSION)/src/callbacks.c \
		$(PACKAGE)-$(VERSION)/src/main.c \
		$(PACKAGE)-$(VERSION)/src/Makefile \
		$(PACKAGE)-$(VERSION)/src/callbacks.h \
		$(PACKAGE)-$(VERSION)/src/common.h \
		$(PACKAGE)-$(VERSION)/src/download.h \
		$(PACKAGE)-$(VERSION)/src/ghtml.h \
		$(PACKAGE)-$(VERSION)/src/surfer.h \
		$(PACKAGE)-$(VERSION)/src/ghtml-gtkhtml.c \
		$(PACKAGE)-$(VERSION)/src/ghtml-gtkmozembed.c \
		$(PACKAGE)-$(VERSION)/src/ghtml-gtktextview.c \
		$(PACKAGE)-$(VERSION)/src/ghtml-webkit.c \
		$(PACKAGE)-$(VERSION)/src/project.conf \
		$(PACKAGE)-$(VERSION)/src/common/Makefile \
		$(PACKAGE)-$(VERSION)/src/common/conn.c \
		$(PACKAGE)-$(VERSION)/src/common/find.c \
		$(PACKAGE)-$(VERSION)/src/common/history.c \
		$(PACKAGE)-$(VERSION)/src/common/url.c \
		$(PACKAGE)-$(VERSION)/src/common/project.conf \
		$(PACKAGE)-$(VERSION)/tools/helper.c \
		$(PACKAGE)-$(VERSION)/tools/Makefile \
		$(PACKAGE)-$(VERSION)/tools/project.conf \
		$(PACKAGE)-$(VERSION)/Makefile \
		$(PACKAGE)-$(VERSION)/COPYING \
		$(PACKAGE)-$(VERSION)/config.h \
		$(PACKAGE)-$(VERSION)/config.sh \
		$(PACKAGE)-$(VERSION)/project.conf
	$(RM) -- $(PACKAGE)-$(VERSION)

distcheck: dist
	$(TAR) -xzvf $(PACKAGE)-$(VERSION).tar.gz
	$(MKDIR) -- $(PACKAGE)-$(VERSION)/objdir
	$(MKDIR) -- $(PACKAGE)-$(VERSION)/destdir
	(cd "$(PACKAGE)-$(VERSION)" && $(MAKE) OBJDIR="$$PWD/objdir/")
	(cd "$(PACKAGE)-$(VERSION)" && $(MAKE) OBJDIR="$$PWD/objdir/" DESTDIR="$$PWD/destdir" install)
	(cd "$(PACKAGE)-$(VERSION)" && $(MAKE) OBJDIR="$$PWD/objdir/" DESTDIR="$$PWD/destdir" uninstall)
	(cd "$(PACKAGE)-$(VERSION)" && $(MAKE) OBJDIR="$$PWD/objdir/" distclean)
	(cd "$(PACKAGE)-$(VERSION)" && $(MAKE) dist)
	$(RM) -r -- $(PACKAGE)-$(VERSION)

install:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) install) || exit; done

uninstall:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) uninstall) || exit; done

.PHONY: all subdirs clean distclean dist distcheck install uninstall
