DeforaOS Surfer
===============

About Surfer
------------

Surfer is a web browser.

Surfer is part of the DeforaOS Project, found at https://www.defora.org/.


Compiling Surfer
-----------------

Surfer depends on the following components:

 * Gtk+ 2.4 or newer, or Gtk+ 3.0 or newer
 * DeforaOS libDesktop
 * an implementation of `make`
 * gettext (libintl) for translations
 * DocBook-XSL for the manual pages
 * GTK-Doc for the API documentation
 * Either one of:
   * WebKit for Gtk+ 2
   * WebKit for Gtk+ 3
   * WebKit2 for Gtk+ 3

With these installed, the following command should be enough to compile and
install Surfer on most systems:

    $ make install

To install (or package) Surfer in a different location, use the `PREFIX` option
as follows:

    $ make PREFIX="/another/prefix" install

Surfer also supports `DESTDIR`, to be installed in a staging directory; for
instance:

    $ make DESTDIR="/staging/directory" PREFIX="/another/prefix" install

The compilation process supports a number of other options, such as OBJDIR for
compilation outside of the source tree for instance.

On some systems, the Makefiles shipped may have to be re-generated accordingly.
This can be performed with the DeforaOS configure tool.

Documentation
-------------

Manual pages for each of the executables installed are available in the `doc`
folder. They are written in the DocBook-XML format, and need libxslt and
DocBook-XSL to be installed for conversion to the HTML or man file format.

Distributing Surfer
-------------------

DeforaOS Surfer is subject to the terms of the GPL license, version 3. Please
see the `COPYING` file for more information.
