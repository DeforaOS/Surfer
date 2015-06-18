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



#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <locale.h>
#include <libintl.h>
#include <gtk/gtk.h>
#include <System.h>
#include "../config.h"
#define _(string) gettext(string)

/* constants */
#ifndef PROGNAME
# define PROGNAME	"bookmark"
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


/* bookmark */
/* private */
/* prototypes */
static int _bookmark(char const * title, char const * url, char const * icon,
		char const * comment);

static int _error(char const * message, int ret);
static int _usage(void);


/* functions */
/* bookmark */
static int _bookmark_do(char const * title, char const * url, char const * icon,
		char const * comment);

static int _bookmark(char const * title, char const * url, char const * icon,
		char const * comment)
{
	int ret;

	if((ret = _bookmark_do(title, url, icon, comment)) != 0)
		error_print(PROGNAME);
	return ret;
}

static int _bookmark_do(char const * title, char const * url, char const * icon,
		char const * comment)
{
	int ret = 0;
	const char section[] = "Desktop Entry";
	char const * homedir;
	String * datahome;
	String * pathname = NULL;
	String * filename;
	Config * config;
	struct stat st;

	if((homedir = getenv("XDG_DATA_HOME")) != NULL)
		datahome = string_new(homedir);
	else
	{
		if((homedir = getenv("HOME")) == NULL)
			homedir = g_get_home_dir();
		datahome = string_new_append(homedir,
				"/.local/share/applications", NULL);
	}
	if(datahome == NULL)
		return -1;
	if(mkdir(datahome, 0755) != 0 && errno != EEXIST)
		ret = error_set_code(1, "%s: %s", datahome, strerror(errno));
	if(ret != 0)
	{
		string_delete(datahome);
		return -1;
	}
	if(title == NULL)
		title = url;
	if((filename = string_new(title)) == NULL)
		return -1;
	if(string_replace(&filename, "/", "_") != 0
			|| (pathname = string_new_append(datahome, "/",
					filename, ".desktop", NULL)) == NULL
			|| (config = config_new()) == NULL)
	{
		string_delete(pathname);
		string_delete(filename);
		string_delete(datahome);
		return -1;
	}
	if(stat(pathname, &st) == 0)
		ret = -error_set_code(1, "%s: %s", title,
				"Bookmark already set");
	else if((ret = config_set(config, section, "Type", "URL")) != 0
			|| (ret = config_set(config, section, "Name", title))
			|| (ret = config_set(config, section, "URL", url)) != 0
			|| (ret = config_set(config, section, "Icon", icon))
			|| (ret = config_set(config, section, "Comment",
					comment)) != 0
			|| (ret = config_save(config, pathname)) != 0)
		ret = -1;
	config_delete(config);
	string_delete(filename);
	string_delete(pathname);
	string_delete(datahome);
	return ret;
}


/* error */
static int _error(char const * message, int ret)
{
	fputs(PROGNAME ": ", stderr);
	perror(message);
	return ret;
}


/* usage */
static int _usage(void)
{
	fprintf(stderr, _("Usage: %s -u [-t title][-i icon][-C comment] URL\n"),
			PROGNAME);
	return 1;
}


/* public */
/* main */
int main(int argc, char * argv[])
{
	int o;
	int url = 0;
	char const * title = NULL;
	char const * icon = "stock_internet";
	char const * comment = NULL;

	if(setlocale(LC_ALL, "") == NULL)
		_error("setlocale", 1);
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	while((o = getopt(argc, argv, "uC:i:t:")) != -1)
		switch(o)
		{
			case 'C':
				comment = optarg;
				break;
			case 'i':
				icon = optarg;
				break;
			case 't':
				title = optarg;
				break;
			case 'u':
				url = 1;
				break;
			default:
				return _usage();
		}
	if((optind + 1) != argc || url == 0)
		return _usage();
	return (_bookmark(title, argv[optind], icon, comment) == 0) ? 0 : 2;
}
