/* $Id$ */
/* Copyright (c) 2015 Pierre Pronchery <khorben@defora.org> */
/* This file is part of DeforaOS Desktop Surfer */
/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. */



#include <unistd.h>
#include <stdio.h>
#include <locale.h>
#include <libintl.h>
#include <gtk/gtk.h>
#include "helper.h"
#include "../config.h"
#define _(string) gettext(string)
#define N_(string) string

/* constants */
#ifndef PROGNAME
# define PROGNAME	"helper"
#endif
#ifndef PREFIX
# define PREFIX		"/usr/local"
#endif
#ifndef DATADIR
# define DATADIR	PREFIX "/share"
#endif
#ifndef LOCALEDIR
# define LOCALEDIR	DATADIR "/locale"
#endif


/* helper */
/* private */
/* prototypes */
static int _usage(void);


/* functions */
/* usage */
static int _usage(void)
{
	fprintf(stderr, _("Usage: %s [-c][-p package] command\n"
"       %s -d package\n"
"       %s -s section page\n"
"  -c	Open the documentation for a specific package\n"
"  -d	Open an API reference\n"
"  -p	Set the package for the command specified\n"
"  -s	Section of the manual page to read from\n"),
			PROGNAME, PROGNAME, PROGNAME);
	return 1;
}


/* public */
/* functions */
/* main */
int main(int argc, char * argv[])
{
	int o;
	int devel = 0;
	char const * package = NULL;
	char const * section = NULL;
	Helper * helper;

	if(setlocale(LC_ALL, "") == NULL)
		helper_error(NULL, "setlocale", 1);
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
#if defined(WITH_GTKHTML) || defined(WITH_GTKTEXTVIEW) || defined(WITH_WEBKIT)
	if(g_thread_supported() == FALSE)
		g_thread_init(NULL);
#endif
	gtk_init(&argc, &argv);
	while((o = getopt(argc, argv, "cdp:s:")) != -1)
		switch(o)
		{
			case 'c':
				section = NULL;
				devel = 0;
				break;
			case 'd':
				section = NULL;
				devel = 1;
				break;
			case 'p':
				package = optarg;
				break;
			case 's':
				section = optarg;
				break;
			default:
				return _usage();
		}
	if(optind != argc && (optind + 1) != argc)
		return _usage();
	if((helper = helper_new()) == NULL)
		return 2;
	if(section != NULL)
		helper_open_manual(helper, section, argv[optind], NULL);
	else if(argv[optind] != NULL && devel != 0)
		helper_open_gtkdoc(helper, NULL, argv[optind]);
	else if(argv[optind] != NULL)
		helper_open_contents(helper, (package != NULL) ? package
				: argv[optind], argv[optind]);
	gtk_main();
	helper_delete(helper);
	return 0;
}
