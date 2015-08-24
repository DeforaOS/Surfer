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
/* TODO:
 * - add a flag to disable network access
 * - add a flag to disable javascript */



#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <locale.h>
#include <libintl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <Desktop.h>
#include "ghtml.h"
#include <System.h>
#include "../config.h"
#define _(string) gettext(string)
#define N_(string) string

/* constants */
#ifndef PROGNAME
# define PROGNAME	"htmlapp"
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


/* htmlapp */
/* private */
/* types */
typedef struct _Surfer
{
	guint source;
	char const ** p;

	/* widgets */
	GtkIconTheme * icontheme;
	GtkWidget * window;
	GtkWidget * vbox;
	GtkWidget * view;

	/* find */
	GtkWidget * fi_dialog;
	GtkWidget * fi_text;
	GtkWidget * fi_case;
	GtkWidget * fi_back;
	GtkWidget * fi_wrap;

	/* about */
	GtkWidget * ab_window;
} HTMLApp;


/* prototypes */
static HTMLApp * _htmlapp_new(void);
void _htmlapp_delete(HTMLApp * htmlapp);

static int _htmlapp_open(HTMLApp * htmlapp, char const * url);
static int _htmlapp_open_dialog(HTMLApp * htmlapp);

static int _error(char const * message, int ret);
static int _usage(void);

/* callbacks */
static void _htmlapp_on_close(gpointer data);
static gboolean _htmlapp_on_closex(gpointer data);
static void _htmlapp_on_find(gpointer data);
static void _htmlapp_on_fullscreen(gpointer data);
static void _htmlapp_on_open(gpointer data);


/* constants */
static const DesktopAccel _htmlapp_accel[] =
{
	{ G_CALLBACK(_htmlapp_on_close), GDK_CONTROL_MASK, GDK_KEY_W },
	{ G_CALLBACK(_htmlapp_on_find), GDK_CONTROL_MASK, GDK_KEY_F },
	{ G_CALLBACK(_htmlapp_on_fullscreen), 0, GDK_KEY_F11 },
	{ G_CALLBACK(_htmlapp_on_open), GDK_CONTROL_MASK, GDK_KEY_O },
	{ NULL, 0, 0 }
};


/* functions */
/* HTMLApp */
/* htmlapp_new */
static HTMLApp * _htmlapp_new(void)
{
	HTMLApp * htmlapp;
	GtkAccelGroup * group;
	GtkWidget * vbox;

	if((htmlapp = surfer_new(NULL)) == NULL)
		return NULL;
	htmlapp->source = 0;
	htmlapp->p = NULL;
	/* widgets */
	htmlapp->icontheme = gtk_icon_theme_get_default();
	/* window */
	group = gtk_accel_group_new();
	htmlapp->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_add_accel_group(GTK_WINDOW(htmlapp->window), group);
#ifndef EMBEDDED
	gtk_window_set_default_size(GTK_WINDOW(htmlapp->window), 640, 480);
#else
	gtk_window_set_default_size(GTK_WINDOW(htmlapp->window), 240, 480);
#endif
#if GTK_CHECK_VERSION(2, 6, 0)
	gtk_window_set_icon_name(GTK_WINDOW(htmlapp->window), "web-browser");
#endif
	gtk_window_set_title(GTK_WINDOW(htmlapp->window), _("HTML Application"));
	g_signal_connect_swapped(htmlapp->window, "delete-event", G_CALLBACK(
				_htmlapp_on_closex), htmlapp);
#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
	vbox = gtk_vbox_new(FALSE, 0);
#endif
	htmlapp->vbox = vbox;
	desktop_accel_create(_htmlapp_accel, htmlapp, group);
	/* view */
	htmlapp->view = ghtml_new(htmlapp);
	ghtml_set_enable_javascript(htmlapp->view, FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), htmlapp->view, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(htmlapp->window), vbox);
	gtk_widget_grab_focus(htmlapp->view);
	gtk_widget_show_all(htmlapp->window);
	htmlapp->fi_dialog = NULL;
	htmlapp->ab_window = NULL;
	return htmlapp;
}


/* htmlapp_delete */
void _htmlapp_delete(HTMLApp * htmlapp)
{
	if(htmlapp->source != 0)
		g_source_remove(htmlapp->source);
	if(htmlapp->ab_window != NULL)
		gtk_widget_destroy(htmlapp->ab_window);
	if(htmlapp->fi_dialog != NULL)
		gtk_widget_destroy(htmlapp->fi_dialog);
	gtk_widget_destroy(htmlapp->window);
	surfer_delete(htmlapp);
}


/* useful */
/* htmlapp_open */
static int _htmlapp_open(HTMLApp * htmlapp, char const * url)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\")\n", __func__, url);
#endif
	if(url == NULL)
		return _htmlapp_open_dialog(htmlapp);
	ghtml_load_url(htmlapp->view, url);
	return 0;
}


/* htmlapp_open_dialog */
static void _open_dialog_on_choose_activate(GtkWidget * widget, gint arg1,
		gpointer data);

static int _htmlapp_open_dialog(HTMLApp * htmlapp)
{
	int ret;
	GtkWidget * dialog;
	GtkWidget * vbox;
	GtkWidget * hbox;
	GtkWidget * entry;
	GtkWidget * widget;
	GtkFileFilter * filter;
	char const * url = NULL;

	dialog = gtk_dialog_new_with_buttons(_("Open page..."),
			GTK_WINDOW(htmlapp->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
#if GTK_CHECK_VERSION(2, 14, 0)
	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
#else
	vbox = GTK_DIALOG(dialog)->vbox;
#endif
	gtk_box_set_spacing(GTK_BOX(vbox), 4);
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	widget = gtk_label_new(_("Filename: "));
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
	/* file chooser */
	widget = gtk_file_chooser_dialog_new(_("Open file..."),
			GTK_WINDOW(dialog), GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("HTML files"));
	gtk_file_filter_add_mime_type(filter, "text/html");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(widget), filter);
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(widget), filter);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("Text files"));
	gtk_file_filter_add_mime_type(filter, "text/plain");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(widget), filter);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("All files"));
	gtk_file_filter_add_pattern(filter, "*");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(widget), filter);
	g_signal_connect(widget, "response", G_CALLBACK(
				_open_dialog_on_choose_activate), entry);
	widget = gtk_file_chooser_button_new_with_dialog(widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_widget_show_all(vbox);
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
		url = gtk_entry_get_text(GTK_ENTRY(entry));
	gtk_widget_hide(dialog);
	if(url == NULL || strlen(url) == 0)
		ret = -1;
	else
		ret = _htmlapp_open(htmlapp, url);
	gtk_widget_destroy(dialog);
	return ret;
}

static void _open_dialog_on_choose_activate(GtkWidget * widget, gint arg1,
		gpointer data)
{
	GtkWidget * entry = data;

	if(arg1 != GTK_RESPONSE_ACCEPT)
		return;
	gtk_entry_set_text(GTK_ENTRY(entry), gtk_file_chooser_get_filename(
				GTK_FILE_CHOOSER(widget)));
}


/* callbacks */
/* htmlapp_on_close */
static void _htmlapp_on_close(gpointer data)
{
	HTMLApp * htmlapp = data;

	gtk_widget_hide(htmlapp->window);
	gtk_main_quit();
}


/* htmlapp_on_closex */
static gboolean _htmlapp_on_closex(gpointer data)
{
	HTMLApp * htmlapp = data;

	_htmlapp_on_close(htmlapp);
	return TRUE;
}


/* htmlapp_on_find */
static void _htmlapp_on_find(gpointer data)
{
	HTMLApp * htmlapp = data;

	surfer_find(htmlapp, NULL);
}


/* htmlapp_on_fullscreen */
static void _htmlapp_on_fullscreen(gpointer data)
{
	HTMLApp * htmlapp = data;
	GdkWindow * window;
	gboolean fullscreen;

#if GTK_CHECK_VERSION(2, 14, 0)
	window = gtk_widget_get_window(htmlapp->window);
#else
	window = htmlapp->window->window;
#endif
	fullscreen = (gdk_window_get_state(window)
			& GDK_WINDOW_STATE_FULLSCREEN)
		== GDK_WINDOW_STATE_FULLSCREEN;
	surfer_set_fullscreen(htmlapp, !fullscreen);
}


/* htmlapp_on_open */
static void _htmlapp_on_open(gpointer data)
{
	HTMLApp * htmlapp = data;

	_htmlapp_open_dialog(htmlapp);
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
	fprintf(stderr, _("Usage: %s [-Jj] [URL]\n"
"  -J	Disable Javascript\n"
"  -j	Enable Javascript (default)\n"), PROGNAME);
	return 1;
}


/* public */
/* surfer */
/* essential */
/* surfer_new */
HTMLApp * surfer_new(char const * url)
{
	HTMLApp * htmlapp;

	if((htmlapp = object_new(sizeof(*htmlapp))) == NULL)
		return NULL;
	return htmlapp;
}


/* surfer_delete */
void surfer_delete(HTMLApp * htmlapp)
{
	object_delete(htmlapp);
}


/* accessors */
/* surfer_get_view */
GtkWidget * surfer_get_view(Surfer * surfer)
{
	/* FIXME remove from the API? */
	return surfer->view;
}


/* surfer_set_enable_javascript */
void surfer_set_enable_javascript(Surfer * surfer, gboolean enable)
{
	ghtml_set_enable_javascript(surfer->view, enable);
}


/* surfer_set_favicon */
void surfer_set_favicon(Surfer * surfer, GdkPixbuf * pixbuf)
{
	/* FIXME implement */
}


/* surfer_set_fullscreen */
void surfer_set_fullscreen(Surfer * surfer, gboolean fullscreen)
{
	HTMLApp * htmlapp = surfer;

	if(fullscreen)
		gtk_window_fullscreen(GTK_WINDOW(htmlapp->window));
	else
		gtk_window_unfullscreen(GTK_WINDOW(htmlapp->window));
}


/* surfer_set_location */
void surfer_set_location(Surfer * surfer, char const * location)
{
	/* FIXME implement? */
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\")\n", __func__, location);
#endif
}


/* surfer_set_progress */
void surfer_set_progress(Surfer * surfer, gdouble fraction)
{
	/* FIXME implement? */
}


/* surfer_set_security */
void surfer_set_security(Surfer * surfer, SurferSecurity security)
{
	/* FIXME implement? */
}


/* surfer_set_status */
void surfer_set_status(Surfer * surfer, char const * status)
{
	/* FIXME really implement? */
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\")\n", __func__, status);
#endif
}


/* surfer_set_title */
void surfer_set_title(Surfer * surfer, char const * title)
{
	HTMLApp * htmlapp = surfer;
	gchar * t;

	t = g_strdup_printf("%s%s%s", _("HTML Application"),
			(title != NULL) ? " - " : "",
			(title != NULL) ? title : "");
	gtk_window_set_title(GTK_WINDOW(htmlapp->window), t);
	g_free(t);
}


/* useful */
/* surfer_confirm */
int surfer_confirm(Surfer * surfer, char const * message, gboolean * confirmed)
{
	HTMLApp * htmlapp = surfer;
	int ret = 0;
	GtkWidget * dialog;
	int res;

	dialog = gtk_message_dialog_new((htmlapp != NULL)
			? GTK_WINDOW(htmlapp->window) : NULL,
			GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
#if GTK_CHECK_VERSION(2, 6, 0)
			"%s", _("Question"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
#endif
			"%s", message);
	gtk_window_set_title(GTK_WINDOW(dialog), _("Question"));
	res = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	if(res == GTK_RESPONSE_YES)
		*confirmed = TRUE;
	else if(res == GTK_RESPONSE_NO)
		*confirmed = FALSE;
	else
		ret = 1;
	return ret;
}


/* surfer_console_message */
void surfer_console_message(Surfer * surfer, char const * message,
		char const * source, long line)
{
	/* FIXME really implement */
	fprintf(stderr, "%s: %s:%ld: %s\n", PROGNAME, source, line, message);
}


/* surfer_copy */
void surfer_copy(Surfer * surfer)
{
	/* FIXME really implement */
	ghtml_copy(surfer->view);
}


/* surfer_download */
int surfer_download(Surfer * surfer, char const * url, char const * suggested)
{
	/* FIXME really implement */
	return 0;
}


/* surfer_error */
int surfer_error(Surfer * surfer, char const * message, int ret)
{
	HTMLApp * htmlapp = surfer;
	GtkWidget * dialog;

	dialog = gtk_message_dialog_new((htmlapp != NULL)
			? GTK_WINDOW(htmlapp->window) : NULL,
			GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
#if GTK_CHECK_VERSION(2, 6, 0)
			"%s", _("Error"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
#endif
			"%s", (message != NULL) ? message : _("Unknown error"));
	gtk_window_set_title(GTK_WINDOW(dialog), _("Error"));
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	return ret;
}


/* surfer_find */
#include "../src/common/find.c"


/* surfer_go_back */
gboolean surfer_go_back(Surfer * surfer)
{
	return ghtml_go_back(surfer->view);
}


/* surfer_go_forward */
gboolean surfer_go_forward(Surfer * surfer)
{
	return ghtml_go_forward(surfer->view);
}


/* surfer_open */
void surfer_open(Surfer * surfer, char const * url)
{
}


/* surfer_open_tab */
void surfer_open_tab(Surfer * surfer, char const * url)
{
	surfer_open(surfer, url);
}


/* surfer_prompt */
int surfer_prompt(Surfer * surfer, char const * message,
		char const * default_value, char ** value)
{
	HTMLApp * htmlapp = surfer;
	int ret = 0;
	GtkWidget * dialog;
	GtkWidget * vbox;
	GtkWidget * entry;
	int res;

	dialog = gtk_message_dialog_new((htmlapp != NULL)
			? GTK_WINDOW(htmlapp->window) : NULL,
			GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL,
#if GTK_CHECK_VERSION(2, 6, 0)
			"%s", _("Question"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
#endif
			"%s", message);
	gtk_window_set_title(GTK_WINDOW(dialog), _("Question"));
#if GTK_CHECK_VERSION(2, 14, 0)
	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
#else
	vbox = GTK_DIALOG(dialog)->vbox;
#endif
	entry = gtk_entry_new();
	if(default_value != NULL)
		gtk_entry_set_text(GTK_ENTRY(entry), default_value);
	gtk_widget_show(entry);
	gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, TRUE, 0);
	if((res = gtk_dialog_run(GTK_DIALOG(dialog))) == GTK_RESPONSE_OK)
		*value = strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
	if(res != GTK_RESPONSE_OK || value == NULL)
		ret = 1;
	gtk_widget_destroy(dialog);
	return ret;
}


/* surfer_print */
void surfer_print(Surfer * surfer)
{
	ghtml_print(surfer->view);
}


/* surfer_refresh */
void surfer_refresh(Surfer * surfer)
{
	ghtml_refresh(surfer->view);
}


/* surfer_resize */
void surfer_resize(Surfer * surfer, gint width, gint height)
{
	HTMLApp * htmlapp = surfer;

	gtk_window_resize(GTK_WINDOW(htmlapp->window), width, height);
}


/* surfer_save_dialog */
void surfer_save_dialog(Surfer * surfer)
{
}


/* surfer_select_all */
void surfer_select_all(Surfer * surfer)
{
	ghtml_select_all(surfer->view);
}


/* surfer_show_console */
void surfer_show_console(Surfer * surfer, gboolean show)
{
}


/* surfer_show_location */
void surfer_show_location(Surfer * surfer, gboolean show)
{
}


/* surfer_show_menubar */
void surfer_show_menubar(Surfer * surfer, gboolean show)
{
}


/* surfer_show_statusbar */
void surfer_show_statusbar(Surfer * surfer, gboolean show)
{
}


/* surfer_show_toolbar */
void surfer_show_toolbar(Surfer * surfer, gboolean show)
{
}


/* surfer_show_window */
void surfer_show_window(Surfer * surfer, gboolean show)
{
}


/* surfer_view_source */
void surfer_view_source(Surfer * surfer)
{
}


/* surfer_warning */
void surfer_warning(Surfer * surfer, char const * message)
{
	fprintf(stderr, "%s: %s\n", PROGNAME, message);
}


/* htmlapp */
/* main */
int main(int argc, char * argv[])
{
	int o;
	HTMLApp * htmlapp;
	gboolean javascript = TRUE;

	if(setlocale(LC_ALL, "") == NULL)
		_error("setlocale", 1);
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
#if defined(WITH_GTKHTML) || defined(WITH_GTKTEXTVIEW) || defined(WITH_WEBKIT)
	if(g_thread_supported() == FALSE)
		g_thread_init(NULL);
#endif
	gtk_init(&argc, &argv);
	while((o = getopt(argc, argv, "jJ")) != -1)
		switch(o)
		{
			case 'J':
				javascript = FALSE;
				break;
			case 'j':
				javascript = TRUE;
				break;
			default:
				return _usage();
		}
	if(optind != argc && (optind + 1) != argc)
		return _usage();
	if((htmlapp = _htmlapp_new()) == NULL)
		return 2;
	surfer_set_enable_javascript(htmlapp, javascript);
	if(argv[optind] != NULL)
		_htmlapp_open(htmlapp, argv[optind]);
	else
		_htmlapp_open_dialog(htmlapp);
	gtk_main();
	_htmlapp_delete(htmlapp);
	return 0;
}
