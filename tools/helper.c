/* $Id$ */
static char const _copyright[] =
"Copyright © 2012-2015 Pierre Pronchery <khorben@defora.org>";
/* This file is part of DeforaOS Desktop Surfer */
static char const _license[] =
"This program is free software: you can redistribute it and/or modify\n"
"it under the terms of the GNU General Public License as published by\n"
"the Free Software Foundation, version 3 of the License.\n"
"\n"
"This program is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"GNU General Public License for more details.\n"
"\n"
"You should have received a copy of the GNU General Public License\n"
"along with this program.  If not, see <http://www.gnu.org/licenses/>.";



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
#ifndef CONTENTSDIR
# define CONTENTSDIR	DATADIR "/doc/html"
#endif
#ifndef MANDIR
# define MANDIR		DATADIR "/man"
#endif


/* helper */
/* private */
/* types */
struct _Surfer
{
	guint source;
	char const ** p;

	/* widgets */
	GtkIconTheme * icontheme;
	GtkWidget * window;
	GtkWidget * vbox;
#ifndef EMBEDDED
	GtkWidget * menubar;
#endif
	GtkWidget * notebook;
	GtkTreeStore * store;
	GtkWidget * contents;
	GtkWidget * gtkdoc;
	GtkWidget * manual;
	GtkWidget * entry;
	GtkWidget * search;
	GtkWidget * view;
	GtkToolItem * tb_fullscreen;

	/* find */
	GtkWidget * fi_dialog;
	GtkWidget * fi_text;
	GtkWidget * fi_case;
	GtkWidget * fi_back;
	GtkWidget * fi_wrap;

	/* about */
	GtkWidget * ab_window;
};

typedef enum _HelperStoreType
{
	HST_CONTENTS = 0,
	HST_GTKDOC,
	HST_MANUAL
} HelperStoreType;

typedef enum _HelperStoreColumn
{
	HSC_TYPE = 0,
	HSC_ICON,
	HSC_DISPLAY,
	HSC_GTKDOC_DIRECTORY,
	HSC_MANUAL_SECTION,
	HSC_MANUAL_DIRECTORY
} HelperStoreColumn;
#define HSC_CONTENTS_PACKAGE HSC_DISPLAY
#define HSC_GTKDOC_PACKAGE HSC_DISPLAY
#define HSC_MANUAL_FILENAME HSC_DISPLAY
#define HSC_LAST HSC_MANUAL_DIRECTORY
#define HSC_COUNT (HSC_LAST + 1)


/* prototypes */
static int _error(char const * message, int ret);

/* callbacks */
static void _helper_on_back(gpointer data);
static void _helper_on_close(gpointer data);
#ifdef EMBEDDED
static void _helper_on_find(gpointer data);
#endif
static void _helper_on_forward(gpointer data);
static gboolean _helper_on_closex(gpointer data);
#ifndef EMBEDDED
static void _helper_on_edit_copy(gpointer data);
static void _helper_on_edit_find(gpointer data);
static void _helper_on_edit_select_all(gpointer data);
static void _helper_on_file_close(gpointer data);
static void _helper_on_file_open(gpointer data);
#endif
static void _helper_on_fullscreen(gpointer data);
#ifndef EMBEDDED
static void _helper_on_help_about(gpointer data);
static void _helper_on_help_contents(gpointer data);
#endif
#ifdef EMBEDDED
static void _helper_on_open(gpointer data);
#endif
#ifndef EMBEDDED
static void _helper_on_view_collapse_all(gpointer data);
static void _helper_on_view_expand_all(gpointer data);
static void _helper_on_view_fullscreen(gpointer data);
#endif

#include "backend/manual.c"
#include "backend/contents.c"
#include "backend/gtkdoc.c"
#include "backend/search.c"


/* constants */
#ifndef EMBEDDED
static char const * _authors[] =
{
	"Pierre Pronchery <khorben@defora.org>",
	NULL
};
#endif

static const DesktopAccel _helper_accel[] =
{
#ifdef EMBEDDED
	{ G_CALLBACK(_helper_on_close), GDK_CONTROL_MASK, GDK_KEY_W },
	{ G_CALLBACK(_helper_on_find), GDK_CONTROL_MASK, GDK_KEY_F },
#endif
	{ G_CALLBACK(_helper_on_fullscreen), 0, GDK_KEY_F11 },
#ifdef EMBEDDED
	{ G_CALLBACK(_helper_on_open), GDK_CONTROL_MASK, GDK_KEY_O },
#endif
	{ NULL, 0, 0 }
};

static DesktopToolbar _helper_toolbar[] =
{
	{ N_("Back"), G_CALLBACK(_helper_on_back), GTK_STOCK_GO_BACK,
		GDK_MOD1_MASK, GDK_KEY_Left, NULL },
	{ N_("Forward"), G_CALLBACK(_helper_on_forward), GTK_STOCK_GO_FORWARD,
		GDK_MOD1_MASK, GDK_KEY_Right, NULL },
	{ "", NULL, NULL, 0, 0, NULL },
	{ NULL, NULL, NULL, 0, 0, NULL }
};

#ifndef EMBEDDED
static const DesktopMenu _menu_file[] =
{
	{ N_("_Open..."), G_CALLBACK(_helper_on_file_open), GTK_STOCK_OPEN,
		GDK_CONTROL_MASK, GDK_KEY_O },
	{ "", NULL, NULL, 0, 0 },
	{ N_("_Close"), G_CALLBACK(_helper_on_file_close), GTK_STOCK_CLOSE,
		GDK_CONTROL_MASK, GDK_KEY_W },
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenu _menu_edit[] =
{
	{ N_("Cop_y"), G_CALLBACK(_helper_on_edit_copy), GTK_STOCK_COPY,
		GDK_CONTROL_MASK, GDK_KEY_C },
	{ "", NULL, NULL, 0, 0 },
	{ N_("Select _All"), G_CALLBACK(_helper_on_edit_select_all),
# if GTK_CHECK_VERSION(2, 10, 0)
		GTK_STOCK_SELECT_ALL,
# else
		NULL,
# endif
		GDK_CONTROL_MASK, GDK_KEY_A },
	{ "", NULL, NULL, 0, 0 },
	{ N_("_Find"), G_CALLBACK(_helper_on_edit_find), GTK_STOCK_FIND,
		GDK_CONTROL_MASK, GDK_KEY_F },
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenu _menu_view[] =
{
	{ N_("_Expand all"), G_CALLBACK(_helper_on_view_expand_all), NULL,
		0, 0 },
	{ N_("_Collapse all"), G_CALLBACK(_helper_on_view_collapse_all), NULL,
		0, 0 },
	{ "", NULL, NULL, 0, 0 },
	{ N_("_Fullscreen"), G_CALLBACK(_helper_on_view_fullscreen),
# if GTK_CHECK_VERSION(2, 8, 0)
		GTK_STOCK_FULLSCREEN, 0, 0 },
# else
		NULL, 0, 0 },
# endif
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenu _menu_help[] =
{
	{ N_("_Contents"), G_CALLBACK(_helper_on_help_contents),
		"help-contents", 0, GDK_KEY_F1 },
	{ N_("_About"), G_CALLBACK(_helper_on_help_about),
# if GTK_CHECK_VERSION(2, 6, 0)
		GTK_STOCK_ABOUT, 0, 0 },
# else
		NULL, 0, 0 },
# endif
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenubar _helper_menubar[] =
{
	{ N_("_File"), _menu_file },
	{ N_("_Edit"), _menu_edit },
	{ N_("_View"), _menu_view },
	{ N_("_Help"), _menu_help },
	{ NULL, NULL }
};
#endif


/* functions */
/* Helper */
/* helper_new */
Helper * helper_new(void)
{
	Helper * helper;
	GtkAccelGroup * group;
	GtkWidget * vbox;
	GtkWidget * widget;
#ifdef EMBEDDED
	GtkToolItem * toolitem;
#endif

	if((helper = surfer_new(NULL)) == NULL)
		return NULL;
	helper->source = 0;
	helper->p = NULL;
	/* widgets */
	helper->icontheme = gtk_icon_theme_get_default();
	/* window */
	group = gtk_accel_group_new();
	helper->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_add_accel_group(GTK_WINDOW(helper->window), group);
	gtk_window_set_default_size(GTK_WINDOW(helper->window), 800, 600);
#if GTK_CHECK_VERSION(2, 6, 0)
	gtk_window_set_icon_name(GTK_WINDOW(helper->window), "help-browser");
#endif
	gtk_window_set_title(GTK_WINDOW(helper->window), _("Help browser"));
	g_signal_connect_swapped(helper->window, "delete-event", G_CALLBACK(
				_helper_on_closex), helper);
#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
	vbox = gtk_vbox_new(FALSE, 0);
#endif
#ifndef EMBEDDED
	/* menubar */
	helper->menubar = desktop_menubar_create(_helper_menubar, helper,
			group);
	gtk_box_pack_start(GTK_BOX(vbox), helper->menubar, FALSE, TRUE, 0);
#endif
	desktop_accel_create(_helper_accel, helper, group);
	/* toolbar */
	widget = desktop_toolbar_create(_helper_toolbar, helper, group);
#ifdef EMBEDDED
	toolitem = gtk_tool_button_new_from_stock(GTK_STOCK_OPEN);
	g_signal_connect_swapped(toolitem, "clicked", G_CALLBACK(
				_helper_on_open), helper);
	gtk_toolbar_insert(GTK_TOOLBAR(widget), toolitem, -1);
	toolitem = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(widget), toolitem, -1);
#endif
#if GTK_CHECK_VERSION(2, 8, 0)
	helper->tb_fullscreen = gtk_toggle_tool_button_new_from_stock(
			GTK_STOCK_FULLSCREEN);
#else
	helper->tb_fullscreen = gtk_toggle_tool_button_new_from_stock(
			GTK_STOCK_ZOOM_FIT);
#endif
	g_signal_connect_swapped(helper->tb_fullscreen, "toggled", G_CALLBACK(
				_helper_on_fullscreen), helper);
	gtk_toolbar_insert(GTK_TOOLBAR(widget), helper->tb_fullscreen, -1);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	/* view */
	widget = gtk_hpaned_new();
	gtk_paned_set_position(GTK_PANED(widget), 150);
	helper->notebook = gtk_notebook_new();
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(helper->notebook), TRUE);
	helper->store = gtk_tree_store_new(HSC_COUNT,
			G_TYPE_UINT,		/* HSC_TYPE		*/
			GDK_TYPE_PIXBUF,	/* HSC_ICON		*/
			G_TYPE_STRING,		/* HSC_DISPLAY		*/
			G_TYPE_STRING,		/* HSC_GTKDOC_DIRECTORY	*/
			G_TYPE_STRING,		/* HSC_MANUAL_SECTION	*/
			G_TYPE_STRING);		/* HSC_MANUAL_FILENAME	*/
	_new_gtkdoc(helper);
	_new_contents(helper);
	_new_manual(helper);
	_new_search(helper);
	gtk_paned_add1(GTK_PANED(widget), helper->notebook);
#if GTK_CHECK_VERSION(3, 0, 0)
	helper->vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
	helper->vbox = gtk_vbox_new(FALSE, 0);
#endif
	helper->view = ghtml_new(helper);
	ghtml_set_enable_javascript(helper->view, FALSE);
	gtk_box_pack_start(GTK_BOX(helper->vbox), helper->view, TRUE, TRUE, 0);
	gtk_paned_add2(GTK_PANED(widget), helper->vbox);
	gtk_box_pack_start(GTK_BOX(vbox), widget, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(helper->window), vbox);
	gtk_widget_grab_focus(helper->view);
	gtk_widget_show_all(helper->window);
	helper->fi_dialog = NULL;
	helper->ab_window = NULL;
	return helper;
}


/* helper_delete */
void helper_delete(Helper * helper)
{
	if(helper->source != 0)
		g_source_remove(helper->source);
	if(helper->ab_window != NULL)
		gtk_widget_destroy(helper->ab_window);
	if(helper->fi_dialog != NULL)
		gtk_widget_destroy(helper->fi_dialog);
	gtk_widget_destroy(helper->window);
	surfer_delete(helper);
}


/* useful */
/* helper_error */
int helper_error(Helper * helper, char const * message, int ret)
{
	return surfer_error(helper, message, ret);
}


/* helper_open */
int helper_open(Helper * helper, char const * url)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\")\n", __func__, url);
#endif
	if(url == NULL)
		return helper_open_dialog(helper);
	ghtml_load_url(helper->view, url);
	return 0;
}


/* helper_open_dialog */
static void _open_dialog_on_entry1_changed(GtkWidget * widget, gpointer data);

int helper_open_dialog(Helper * helper)
{
	int ret;
	GtkSizeGroup * lgroup;
	GtkSizeGroup * group;
	GtkWidget * dialog;
	GtkWidget * vbox;
	GtkWidget * hbox;
	GtkWidget * label;
	GtkWidget * entry1;
	GtkWidget * entry2;
	GtkTreeModel * model;
	char const * package = NULL;
	char const * command;

	lgroup = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	group = gtk_size_group_new(GTK_SIZE_GROUP_BOTH);
	dialog = gtk_dialog_new_with_buttons(_("Open page..."),
			GTK_WINDOW(helper->window),
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
	/* package */
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	label = gtk_label_new(_("Package:"));
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(label, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(lgroup, label);
	gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE, 0);
	model = gtk_tree_model_filter_new(GTK_TREE_MODEL(helper->store), NULL);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(model),
			_helper_filter_contents, NULL, NULL);
#if GTK_CHECK_VERSION(2, 24, 0)
	entry1 = gtk_combo_box_new_with_model_and_entry(model);
	gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(entry1), HSC_DISPLAY);
#else
	entry1 = gtk_combo_box_entry_new_with_model(model, HSC_DISPLAY);
	gtk_combo_box_entry_set_text_column(GTK_COMBO_BOX_ENTRY(entry1),
			HSC_DISPLAY);
#endif
	gtk_size_group_add_widget(group, entry1);
	entry2 = gtk_entry_new();
	gtk_size_group_add_widget(group, entry2);
	g_signal_connect(entry1, "changed", G_CALLBACK(
				_open_dialog_on_entry1_changed), entry2);
	gtk_box_pack_start(GTK_BOX(hbox), entry1, TRUE, TRUE, 0);
	entry1 = gtk_bin_get_child(GTK_BIN(entry1));
	gtk_entry_set_activates_default(GTK_ENTRY(entry1), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* command */
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	label = gtk_label_new(_("Command:"));
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(label, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(lgroup, label);
	gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE, 0);
	gtk_entry_set_activates_default(GTK_ENTRY(entry2), TRUE);
	gtk_box_pack_start(GTK_BOX(hbox), entry2, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_widget_show_all(vbox);
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
	{
		package = gtk_entry_get_text(GTK_ENTRY(entry1));
		command = gtk_entry_get_text(GTK_ENTRY(entry2));
	}
	gtk_widget_hide(dialog);
	if(package == NULL || strlen(package) == 0)
		ret = -1;
	else
		ret = helper_open_contents(helper, package, command);
	gtk_widget_destroy(dialog);
	return ret;
}

static void _open_dialog_on_entry1_changed(GtkWidget * widget, gpointer data)
{
	GtkTreeModel * model;
	GtkTreeIter parent;
	GtkTreeIter iter;
	GtkWidget * entry2 = data;
	gchar * command;

	model = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
	if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter) != TRUE)
		return;
	gtk_tree_model_get(model, &iter, HSC_DISPLAY, &command, -1);
	if(gtk_tree_model_iter_parent(model, &parent, &iter) == TRUE)
	{
		gtk_entry_set_text(GTK_ENTRY(entry2), command);
		g_free(command);
		gtk_tree_model_get(model, &parent, HSC_DISPLAY, &command, -1);
	}
	else
		gtk_entry_set_text(GTK_ENTRY(entry2), "");
	gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(
					GTK_BIN(widget))), command);
	g_free(command);
}


/* callbacks */
/* helper_on_back */
static void _helper_on_back(gpointer data)
{
	Helper * helper = data;

	surfer_go_back(helper);
}


/* helper_on_close */
static void _helper_on_close(gpointer data)
{
	Helper * helper = data;

	gtk_widget_hide(helper->window);
	gtk_main_quit();
}


/* helper_on_closex */
static gboolean _helper_on_closex(gpointer data)
{
	Helper * helper = data;

	_helper_on_close(helper);
	return TRUE;
}


#ifndef EMBEDDED
/* helper_on_edit_copy */
static void _helper_on_edit_copy(gpointer data)
{
	Helper * helper = data;

	surfer_copy(helper);
}


/* helper_on_edit_find */
static void _helper_on_edit_find(gpointer data)
{
	Helper * helper = data;

	surfer_find(helper, NULL);
}


/* helper_on_edit_select_all */
static void _helper_on_edit_select_all(gpointer data)
{
	Helper * helper = data;

	surfer_select_all(helper);
}


/* helper_on_file_close */
static void _helper_on_file_close(gpointer data)
{
	Helper * helper = data;

	gtk_widget_hide(helper->window);
	gtk_main_quit();
}


/* helper_on_file_open */
static void _helper_on_file_open(gpointer data)
{
	Helper * helper = data;

	helper_open_dialog(helper);
}
#endif


#ifdef EMBEDDED
/* helper_on_find */
static void _helper_on_find(gpointer data)
{
	Helper * helper = data;

	surfer_find(helper, NULL);
}
#endif


/* helper_on_forward */
static void _helper_on_forward(gpointer data)
{
	Helper * helper = data;

	surfer_go_forward(helper);
}


/* helper_on_fullscreen */
static void _helper_on_fullscreen(gpointer data)
{
	Helper * helper = data;
	GdkWindow * window;
	gboolean fullscreen;

#if GTK_CHECK_VERSION(2, 14, 0)
	window = gtk_widget_get_window(helper->window);
#else
	window = helper->window->window;
#endif
	fullscreen = (gdk_window_get_state(window)
			& GDK_WINDOW_STATE_FULLSCREEN)
		== GDK_WINDOW_STATE_FULLSCREEN;
	surfer_set_fullscreen(helper, !fullscreen);
}


#ifndef EMBEDDED
/* helper_on_help_about */
static gboolean _about_on_closex(gpointer data);

static void _helper_on_help_about(gpointer data)
{
	Helper * helper = data;

	if(helper->ab_window != NULL)
	{
		gtk_window_present(GTK_WINDOW(helper->ab_window));
		return;
	}
	helper->ab_window = desktop_about_dialog_new();
	gtk_window_set_transient_for(GTK_WINDOW(helper->ab_window), GTK_WINDOW(
				helper->window));
	desktop_about_dialog_set_authors(helper->ab_window, _authors);
	desktop_about_dialog_set_comments(helper->ab_window,
			_("Online help for the DeforaOS desktop"));
	desktop_about_dialog_set_copyright(helper->ab_window, _copyright);
	desktop_about_dialog_set_logo_icon_name(helper->ab_window,
			"help-browser");
	desktop_about_dialog_set_license(helper->ab_window, _license);
	desktop_about_dialog_set_name(helper->ab_window, PACKAGE);
	desktop_about_dialog_set_version(helper->ab_window, VERSION);
	g_signal_connect_swapped(helper->ab_window, "delete-event", G_CALLBACK(
				_about_on_closex), helper);
	gtk_widget_show(helper->ab_window);
}

static gboolean _about_on_closex(gpointer data)
{
	Helper * helper = data;

	gtk_widget_hide(helper->ab_window);
	return TRUE;
}


/* helper_on_help_contents */
static void _helper_on_help_contents(gpointer data)
{
	Helper * helper = data;

	helper_open_manual(helper, "1", PROGNAME, MANDIR);
}
#endif


#ifdef EMBEDDED
/* helper_on_open */
static void _helper_on_open(gpointer data)
{
	Helper * helper = data;

	helper_open_dialog(helper);
}
#endif


#ifndef EMBEDDED
/* helper_on_view_collapse_all */
static void _helper_on_view_collapse_all(gpointer data)
{
	Helper * helper = data;
	int page;
	GtkWidget * widget;

	if((page = gtk_notebook_get_current_page(
					GTK_NOTEBOOK(helper->notebook))) < 0)
		return;
	if((widget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(helper->notebook),
					page)) == NULL)
		return;
	if(!GTK_IS_BIN(widget))
		return;
	if((widget = gtk_bin_get_child(GTK_BIN(widget))) == NULL)
		return;
	gtk_tree_view_collapse_all(GTK_TREE_VIEW(widget));
}


/* helper_on_view_expand_all */
static void _helper_on_view_expand_all(gpointer data)
{
	Helper * helper = data;
	int page;
	GtkWidget * widget;

	if((page = gtk_notebook_get_current_page(
					GTK_NOTEBOOK(helper->notebook))) < 0)
		return;
	if((widget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(helper->notebook),
					page)) == NULL)
		return;
	if(!GTK_IS_BIN(widget))
		return;
	if((widget = gtk_bin_get_child(GTK_BIN(widget))) == NULL)
		return;
	gtk_tree_view_expand_all(GTK_TREE_VIEW(widget));
}


/* helper_on_view_fullscreen */
static void _helper_on_view_fullscreen(gpointer data)
{
	Helper * helper = data;

	_helper_on_fullscreen(helper);
}
#endif


/* error */
static int _error(char const * message, int ret)
{
	fputs(PROGNAME ": ", stderr);
	perror(message);
	return ret;
}


/* public */
/* surfer */
/* essential */
/* surfer_new */
Helper * surfer_new(char const * url)
{
	Helper * helper;

	if((helper = object_new(sizeof(*helper))) == NULL)
		return NULL;
	return helper;
}


/* surfer_delete */
void surfer_delete(Helper * helper)
{
	object_delete(helper);
}


/* accessors */
/* surfer_get_view */
GtkWidget * surfer_get_view(Surfer * surfer)
{
	/* FIXME remove from the API? */
	return surfer->view;
}


/* surfer_set_favicon */
void surfer_set_favicon(Surfer * surfer, GdkPixbuf * pixbuf)
{
	/* FIXME implement */
}


/* surfer_set_fullscreen */
void surfer_set_fullscreen(Surfer * surfer, gboolean fullscreen)
{
	Helper * helper = surfer;

	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(
				helper->tb_fullscreen), fullscreen);
	if(fullscreen)
	{
#ifndef EMBEDDED
		gtk_widget_hide(helper->menubar);
#endif
		gtk_window_fullscreen(GTK_WINDOW(helper->window));
	}
	else
	{
#ifndef EMBEDDED
		gtk_widget_show(helper->menubar);
#endif
		gtk_window_unfullscreen(GTK_WINDOW(helper->window));
	}
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
	Helper * helper = surfer;
	gchar * t;

	t = g_strdup_printf("%s%s%s", _("Help browser"),
			(title != NULL) ? " - " : "",
			(title != NULL) ? title : "");
	gtk_window_set_title(GTK_WINDOW(helper->window), t);
	g_free(t);
}


/* useful */
/* surfer_confirm */
int surfer_confirm(Surfer * surfer, char const * message, gboolean * confirmed)
{
	Helper * helper = surfer;
	int ret = 0;
	GtkWidget * dialog;
	int res;

	dialog = gtk_message_dialog_new((helper != NULL)
			? GTK_WINDOW(helper->window) : NULL,
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
	Helper * helper = surfer;
	GtkWidget * dialog;

	if(surfer == NULL)
		return _error(message, ret);
	dialog = gtk_message_dialog_new((helper != NULL)
			? GTK_WINDOW(helper->window) : NULL,
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
	Helper * helper = surfer;
	int ret = 0;
	GtkWidget * dialog;
	GtkWidget * vbox;
	GtkWidget * entry;
	int res;

	dialog = gtk_message_dialog_new((helper != NULL)
			? GTK_WINDOW(helper->window) : NULL,
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
	Helper * helper = surfer;

	gtk_window_resize(GTK_WINDOW(helper->window), width, height);
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
