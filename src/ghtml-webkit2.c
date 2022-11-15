/* $Id$ */
/* Copyright (c) 2020 Pierre Pronchery <khorben@defora.org> */
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



#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <libintl.h>
#include <webkit2/webkit2.h>
#include "callbacks.h"
#include "ghtml.h"
#include "common/url.c"
#include "../config.h"
#define _(string) gettext(string)


/* private */
/* types */
typedef struct _GHtml
{
	Surfer * surfer;
	GtkWidget * widget;
	GtkWidget * view;
	GtkWidget * inspector;
	char * status;
	gboolean ssl;

	/* for popup menus */
	char * popup_image;
	char * popup_link;
} GHtml;


/* prototypes */
/* functions */
/* accessors */
static void _ghtml_set_status(GtkWidget * widget, char const * status);

/* useful */
static void _ghtml_inspect_url(GtkWidget * widget, char const * url);

/* callbacks */
static gboolean _on_context_menu(WebKitWebView * view, WebKitContextMenu * menu,
		GdkEvent * event, WebKitHitTestResult * result, gpointer data);
static void _on_copy_link_location(gpointer data);
static WebKitWebView * _on_create_web_view(WebKitWebView * view,
		WebKitNavigationAction * action, gpointer data);
static void _on_inspect_page(gpointer data);
static void _on_open_new_tab(gpointer data);
static void _on_open_new_window(gpointer data);
static void _on_save_image_as(gpointer data);
static void _on_save_link_as(gpointer data);
static void _on_web_view_ready(WebKitWebView * view, gpointer data);


/* public */
/* functions */
/* ghtml_new */
GtkWidget * ghtml_new(Surfer * surfer)
{
	GHtml * ghtml;
	GtkWidget * widget;

	if((ghtml = object_new(sizeof(*ghtml))) == NULL)
		return NULL;
	ghtml->surfer = surfer;
	ghtml->status = NULL;
	ghtml->ssl = FALSE;
	ghtml->popup_image = NULL;
	ghtml->popup_link = NULL;
	/* widgets */
	widget = gtk_scrolled_window_new(NULL, NULL);
	ghtml->widget = widget;
	ghtml->view = webkit_web_view_new();
	ghtml->inspector = NULL;
	g_object_set_data(G_OBJECT(widget), "ghtml", ghtml);
	/* view */
	g_signal_connect(ghtml->view, "context-menu", G_CALLBACK(
				_on_context_menu), widget);
	g_signal_connect(ghtml->view, "create", G_CALLBACK(
				_on_create_web_view), widget);
	/* scrolled window */
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(widget), ghtml->view);
	return widget;
}


/* ghtml_delete */
void ghtml_delete(GtkWidget * widget)
{
	GHtml * ghtml;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	free(ghtml->popup_image);
	free(ghtml->popup_link);
	free(ghtml->status);
	object_delete(ghtml);
}


/* accessors */
/* ghtml_can_go_back */
gboolean ghtml_can_go_back(GtkWidget * widget)
{
	GHtml * ghtml;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	return webkit_web_view_can_go_back(WEBKIT_WEB_VIEW(ghtml->view));
}


/* ghtml_get_favicon */
GdkPixbuf * ghtml_get_favicon(GtkWidget * widget)
{
	return NULL;
}


gboolean ghtml_can_go_forward(GtkWidget * widget)
{
	GHtml * ghtml;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	return webkit_web_view_can_go_forward(WEBKIT_WEB_VIEW(ghtml->view));
}


char const * ghtml_get_link_message(GtkWidget * widget)
{
	/* FIXME implement */
	return NULL;
}


/* ghtml_get_location */
char const * ghtml_get_location(GtkWidget * widget)
{
	GHtml * ghtml;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	return webkit_web_view_get_uri(WEBKIT_WEB_VIEW(ghtml->view));
}


/* ghtml_get_progress */
gdouble ghtml_get_progress(GtkWidget * widget)
{
	gdouble ret = -1.0;
	GHtml * ghtml;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	ret = webkit_web_view_get_estimated_load_progress(
			WEBKIT_WEB_VIEW(ghtml->view));
	if(ret == 0.0)
		ret = -1.0;
	return ret;
}


/* ghtml_get_security */
SurferSecurity ghtml_get_security(GtkWidget * widget)
{
	SurferSecurity security = SS_NONE;
	return security;
}


/* ghtml_get_source */
char const * ghtml_get_source(GtkWidget * widget)
{
	return NULL;
}


/* ghtml_get_status */
char const * ghtml_get_status(GtkWidget * widget)
{
	GHtml * ghtml;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	return ghtml->status;
}


/* ghtml_get_title */
char const * ghtml_get_title(GtkWidget * widget)
{
	GHtml * ghtml;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	return webkit_web_view_get_title(ghtml->view);
}


/* ghtml_get_zoom */
gdouble ghtml_get_zoom(GtkWidget * widget)
{
	GHtml * ghtml;
	gdouble zoom;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	zoom = webkit_web_view_get_zoom_level(WEBKIT_WEB_VIEW(ghtml->view));
	return zoom;
}


/* ghtml_set_enable_javascript */
int ghtml_set_enable_javascript(GtkWidget * widget, gboolean enable)
{
	GHtml * ghtml;
	WebKitSettings * settings;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(ghtml->view));
	g_object_set(settings, "enable-scripts", enable, NULL);
	return 0;
}


/* ghtml_set_proxy */
int ghtml_set_proxy(GtkWidget * widget, SurferProxyType type, char const * http,
		uint16_t http_port)
{
	/* FIXME implement */
	return -1;
}


/* ghtml_set_user_agent */
int ghtml_set_user_agent(GtkWidget * ghtml, char const * user_agent)
{
	/* FIXME implement */
	return -1;
}


/* ghtml_set_zoom */
void ghtml_set_zoom(GtkWidget * widget, gdouble zoom)
{
	GHtml * ghtml;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	webkit_web_view_set_zoom_level(WEBKIT_WEB_VIEW(ghtml->view), zoom);
}


/* useful */
/* ghtml_copy */
void ghtml_copy(GtkWidget * widget)
{
	GHtml * ghtml;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	webkit_web_view_execute_editing_command(WEBKIT_WEB_VIEW(ghtml->view),
			WEBKIT_EDITING_COMMAND_COPY);
}


/* ghtml_cut */
void ghtml_cut(GtkWidget * widget)
{
	GHtml * ghtml;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	webkit_web_view_execute_editing_command(WEBKIT_WEB_VIEW(ghtml->view),
			WEBKIT_EDITING_COMMAND_CUT);
}


/* ghtml_execute */
void ghtml_execute(GtkWidget * widget, char const * code)
{
	GHtml * ghtml;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	webkit_web_view_run_javascript(WEBKIT_WEB_VIEW(ghtml->view), code,
			NULL, NULL, NULL);
}


/* ghtml_find */
gboolean ghtml_find(GtkWidget * widget, char const * text, gboolean sensitive,
		gboolean backwards, gboolean wrap)
{
	GHtml * ghtml;
	WebKitFindController * find;
	guint32 options;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	find = webkit_web_view_get_find_controller(
			WEBKIT_WEB_VIEW(ghtml->view));
	options = sensitive ? 0 : WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE;
	options |= backwards ? 0 : WEBKIT_FIND_OPTIONS_BACKWARDS;
	webkit_find_controller_search(find, text, options, G_MAXUINT);
	/* FIXME implement the asynchronous operation */
	return FALSE;
}


gboolean ghtml_go_back(GtkWidget * widget)
{
	GHtml * ghtml;

	if(ghtml_can_go_back(widget) == FALSE)
		return FALSE;
	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	webkit_web_view_go_back(WEBKIT_WEB_VIEW(ghtml->view));
	return TRUE;
}


gboolean ghtml_go_forward(GtkWidget * widget)
{
	GHtml * ghtml;

	if(ghtml_can_go_forward(widget) == FALSE)
		return FALSE;
	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	webkit_web_view_go_forward(WEBKIT_WEB_VIEW(ghtml->view));
	return TRUE;
}


/* ghtml_load_url */
void ghtml_load_url(GtkWidget * widget, char const * url)
{
	GHtml * ghtml;
	char * p;
	char * q = NULL;
	const char about[] = "<!DOCTYPE html>\n<html>\n"
		"<head><title>About " PACKAGE "</title></head>\n"
		"<body>\n<center>\n<h1>" PACKAGE " " VERSION "</h1>\n"
		"<p>Copyright &copy; 2009-2015 Pierre Pronchery &lt;khorben@"
		"defora.org&gt;</p>\n</center>\n</body>\n</html>";

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	if((p = _ghtml_make_url(NULL, url)) != NULL)
		url = p;
	webkit_web_view_load_uri(WEBKIT_WEB_VIEW(ghtml->view), url);
	g_free(p);
	g_free(q);
	surfer_set_progress(ghtml->surfer, 0.0);
	surfer_set_security(ghtml->surfer, SS_NONE);
	_ghtml_set_status(widget, _("Connecting..."));
}


/* ghtml_paste */
void ghtml_paste(GtkWidget * widget)
{
	GHtml * ghtml;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	webkit_web_view_execute_editing_command(WEBKIT_WEB_VIEW(ghtml->view),
			WEBKIT_EDITING_COMMAND_PASTE);
}


/* ghtml_print */
void ghtml_print(GtkWidget * widget)
{
	/* FIXME implement */
}


/* ghtml_redo */
void ghtml_redo(GtkWidget * widget)
{
	GHtml * ghtml;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	webkit_web_view_execute_editing_command(WEBKIT_WEB_VIEW(ghtml->view),
			WEBKIT_EDITING_COMMAND_REDO);
}


void ghtml_refresh(GtkWidget * widget)
{
	GHtml * ghtml;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	webkit_web_view_reload(WEBKIT_WEB_VIEW(ghtml->view));
}


void ghtml_reload(GtkWidget * widget)
{
	GHtml * ghtml;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	webkit_web_view_reload_bypass_cache(WEBKIT_WEB_VIEW(ghtml->view));
}


void ghtml_stop(GtkWidget * widget)
{
	GHtml * ghtml;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	webkit_web_view_stop_loading(WEBKIT_WEB_VIEW(ghtml->view));
}


void ghtml_select_all(GtkWidget * widget)
{
	GHtml * ghtml;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	webkit_web_view_execute_editing_command(WEBKIT_WEB_VIEW(ghtml->view),
			WEBKIT_EDITING_COMMAND_SELECT_ALL);
}


/* ghtml_undo */
void ghtml_undo(GtkWidget * widget)
{
	GHtml * ghtml;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	webkit_web_view_execute_editing_command(WEBKIT_WEB_VIEW(ghtml->view),
			WEBKIT_EDITING_COMMAND_UNDO);
}


/* ghtml_unselect_all */
void ghtml_unselect_all(GtkWidget * widget)
{
	/* FIXME implement */
}


/* ghtml_zoom_in */
void ghtml_zoom_in(GtkWidget * widget)
{
	GHtml * ghtml;
	gdouble zoom;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	zoom = webkit_web_view_get_zoom_level(WEBKIT_WEB_VIEW(ghtml->view));
	webkit_web_view_set_zoom_level(WEBKIT_WEB_VIEW(ghtml->view),
			zoom + 0.1);
}


/* ghtml_zoom_out */
void ghtml_zoom_out(GtkWidget * widget)
{
	GHtml * ghtml;
	gdouble zoom;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	zoom = webkit_web_view_get_zoom_level(WEBKIT_WEB_VIEW(ghtml->view))
		- 0.1;
	if(zoom < 0.1)
		zoom = 0.1;
	webkit_web_view_set_zoom_level(WEBKIT_WEB_VIEW(ghtml->view), zoom);
}


/* ghtml_zoom_reset */
void ghtml_zoom_reset(GtkWidget * widget)
{
	GHtml * ghtml;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	webkit_web_view_set_zoom_level(WEBKIT_WEB_VIEW(ghtml->view), 1.0);
}


/* private */
/* functions */
/* ghtml_set_status */
static void _ghtml_set_status(GtkWidget * widget, char const * status)
{
	GHtml * ghtml;
	gdouble progress;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	free(ghtml->status);
	if(status == NULL)
	{
		if((progress = ghtml_get_progress(widget)) == 0.0)
			status = _("Connecting...");
		else if(progress > 0.0)
			status = _("Downloading...");
	}
	/* XXX may fail */
	ghtml->status = (status != NULL) ? strdup(status) : NULL;
	surfer_set_status(ghtml->surfer, status);
}


/* useful */
static WebKitWebView * _inspect_inspect(WebKitWebInspector * inspector,
		WebKitWebView * view, gpointer data);
static gboolean _inspect_inspected_uri(WebKitWebInspector * inspector,
		gpointer data);
static gboolean _inspect_show(WebKitWebInspector * inspector,
		gpointer data);
/* callbacks */
static gboolean _inspect_on_closex(gpointer data);

static void _ghtml_inspect_url(GtkWidget * widget, char const * url)
{
	GHtml * ghtml;
	WebKitSettings * settings;
	WebKitWebInspector * inspector;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	if(ghtml->inspector == NULL)
	{
		settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(
					ghtml->view));
		g_object_set(settings, "enable-developer-extras", TRUE, NULL);
		surfer_open(ghtml->surfer, url);
		inspector = webkit_web_view_get_inspector(WEBKIT_WEB_VIEW(
					ghtml->view));
		g_signal_connect(inspector, "inspect-web-view", G_CALLBACK(
					_inspect_inspect), ghtml);
		g_signal_connect(inspector, "show-window", G_CALLBACK(
					_inspect_show), ghtml);
		g_signal_connect(inspector, "notify::inspected-uri", G_CALLBACK(
					_inspect_inspected_uri), ghtml);
	}
	else
	{
		surfer_open(ghtml->surfer, url);
		inspector = webkit_web_view_get_inspector(WEBKIT_WEB_VIEW(
					ghtml->view));
	}
	/* FIXME crashes (tested on NetBSD/amd64) */
	webkit_web_inspector_show(inspector);
}

static WebKitWebView * _inspect_inspect(WebKitWebInspector * inspector,
		WebKitWebView * view, gpointer data)
{
	GHtml * ghtml = data;

	ghtml->inspector = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(ghtml->inspector), 800, 600);
	gtk_window_set_title(GTK_WINDOW(ghtml->inspector),
			_("WebKit Web Inspector"));
	g_signal_connect_swapped(ghtml->inspector, "delete-event", G_CALLBACK(
				_inspect_on_closex), ghtml);
	view = webkit_web_view_new();
	/* FIXME implement more signals and really implement "web-view-ready" */
	gtk_container_add(GTK_CONTAINER(ghtml->inspector), GTK_WIDGET(view));
	return view;
}

static gboolean _inspect_show(WebKitWebInspector * inspector, gpointer data)
{
	GHtml * ghtml = data;

	gtk_window_present(GTK_WINDOW(ghtml->inspector));
	return TRUE;
}

static gboolean _inspect_inspected_uri(WebKitWebInspector * inspector,
		gpointer data)
{
	GHtml * ghtml = data;
	char buf[256];
	char const * url;

	url = webkit_web_inspector_get_inspected_uri(inspector);
	snprintf(buf, sizeof(buf), "%s%s%s", _("WebKit Web Inspector"),
			(url != NULL) ? " - " : "", (url != NULL) ? url : "");
	return FALSE;
}

/* callbacks */
static gboolean _inspect_on_closex(gpointer data)
{
	GHtml * ghtml = data;

	gtk_widget_hide(ghtml->inspector);
	return TRUE;
}


/* callbacks */
/* on_context_menu */
static void _context_menu_document(GHtml * ghtml, GtkWidget * menu);
static void _context_menu_editable(GHtml * ghtml);
static void _context_menu_image(GHtml * ghtml, WebKitHitTestResult * result,
		GtkWidget * menu);
static void _context_menu_link(GHtml * ghtml, WebKitHitTestResult * result,
		GtkWidget * menu);
static void _context_menu_media(GHtml * ghtml);
static void _context_menu_position(GtkMenu * menu, gint * x, gint * y,
		gboolean * push, gpointer data);
static void _context_menu_selection(GHtml * ghtml, GtkWidget * menu);
static void _context_menu_separator(GtkWidget * menu, gboolean * separator);

static gboolean _on_context_menu(WebKitWebView * view, WebKitContextMenu * menu,
		GdkEvent * event, WebKitHitTestResult * result, gpointer data)
{
	GtkWidget * widget = data;
	GHtml * ghtml;
	WebKitHitTestResultContext context;
	gboolean separator = FALSE;

	/* FIXME implement every callback */
	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	menu = gtk_menu_new();
	context = webkit_hit_test_result_get_context(result);
	if(context & WEBKIT_HIT_TEST_RESULT_CONTEXT_EDITABLE)
	{
		_context_menu_separator(menu, &separator);
		_context_menu_editable(ghtml);
	}
	if(context & WEBKIT_HIT_TEST_RESULT_CONTEXT_LINK)
	{
		_context_menu_separator(menu, &separator);
		_context_menu_link(ghtml, result, menu);
	}
	if(context & WEBKIT_HIT_TEST_RESULT_CONTEXT_DOCUMENT
			&& !(context & WEBKIT_HIT_TEST_RESULT_CONTEXT_SELECTION)
			&& !(context & WEBKIT_HIT_TEST_RESULT_CONTEXT_LINK))
	{
		_context_menu_separator(menu, &separator);
		_context_menu_document(ghtml, menu);
	}
	if(context & WEBKIT_HIT_TEST_RESULT_CONTEXT_SELECTION)
	{
		_context_menu_separator(menu, &separator);
		_context_menu_selection(ghtml, menu);
	}
	if(context & WEBKIT_HIT_TEST_RESULT_CONTEXT_IMAGE)
	{
		_context_menu_separator(menu, &separator);
		_context_menu_image(ghtml, result, menu);
	}
	if(context & WEBKIT_HIT_TEST_RESULT_CONTEXT_MEDIA)
	{
		_context_menu_separator(menu, &separator);
		_context_menu_media(ghtml);
	}
	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 3,
			gtk_get_current_event_time());
	return TRUE;
}

static void _context_menu_document(GHtml * ghtml, GtkWidget * menu)
{
	GtkWidget * menuitem;
	GtkWidget * image;

	/* back */
	menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_GO_BACK, NULL);
	if(!ghtml_can_go_back(ghtml->widget))
		gtk_widget_set_sensitive(menuitem, FALSE);
	else
		g_signal_connect_swapped(menuitem, "activate", G_CALLBACK(
					surfer_go_back), ghtml->surfer);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	/* forward */
	menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_GO_FORWARD,
			NULL);
	if(!ghtml_can_go_forward(ghtml->widget))
		gtk_widget_set_sensitive(menuitem, FALSE);
	else
		g_signal_connect_swapped(menuitem, "activate", G_CALLBACK(
					surfer_go_forward), ghtml->surfer);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	/* refresh */
	menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_REFRESH, NULL);
	g_signal_connect_swapped(menuitem, "activate", G_CALLBACK(
				surfer_refresh), ghtml->surfer);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	/* separator */
	menuitem = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	/* save page */
	menuitem = gtk_image_menu_item_new_with_mnemonic(
			_("_Save page as..."));
	image = gtk_image_new_from_stock(GTK_STOCK_SAVE_AS, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
	g_signal_connect_swapped(menuitem, "activate", G_CALLBACK(
				surfer_save_dialog), ghtml->surfer);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	/* separator */
	menuitem = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	/* print */
	menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_PRINT, NULL);
	g_signal_connect_swapped(menuitem, "activate", G_CALLBACK(surfer_print),
			ghtml->surfer);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	/* separator */
	menuitem = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	/* select all */
	menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_SELECT_ALL,
			NULL);
	g_signal_connect_swapped(menuitem, "activate", G_CALLBACK(
				surfer_select_all), ghtml->surfer);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	/* separator */
	menuitem = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	/* view source */
	menuitem = gtk_image_menu_item_new_with_mnemonic(_("View so_urce"));
	image = gtk_image_new_from_icon_name("surfer-view-html-source",
			GTK_ICON_SIZE_MENU);
	g_signal_connect_swapped(menuitem, "activate", G_CALLBACK(
				surfer_view_source), ghtml->surfer);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	/* inspect */
	menuitem = gtk_image_menu_item_new_with_mnemonic(
			_("_Inspect this page"));
	g_signal_connect_swapped(menuitem, "activate", G_CALLBACK(
				_on_inspect_page), ghtml);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
}

static void _context_menu_editable(GHtml * ghtml)
{
	/* FIXME implement */
}

static void _context_menu_image(GHtml * ghtml, WebKitHitTestResult * result,
		GtkWidget * menu)
{
	GtkWidget * menuitem;
	GtkWidget * image;

	free(ghtml->popup_image);
	ghtml->popup_image;
	g_object_get(G_OBJECT(result), "image-uri", &ghtml->popup_image, NULL);
	/* view image */
	menuitem = gtk_image_menu_item_new_with_mnemonic(_("_View image"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	/* save image as */
	menuitem = gtk_image_menu_item_new_with_mnemonic(
			_("_Save image as..."));
	image = gtk_image_new_from_stock(GTK_STOCK_SAVE_AS, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
	g_signal_connect_swapped(menuitem, "activate", G_CALLBACK(
				_on_save_image_as), ghtml);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
}

static void _context_menu_link(GHtml * ghtml, WebKitHitTestResult * result,
		GtkWidget * menu)
{
	GtkWidget * menuitem;
	GtkWidget * image;

	free(ghtml->popup_link);
	ghtml->popup_link;
	g_object_get(G_OBJECT(result), "link-uri", &ghtml->popup_link, NULL);
	/* open in new tab */
	menuitem = gtk_image_menu_item_new_with_mnemonic(_("Open in new _tab"));
	image = gtk_image_new_from_icon_name("tab-new", GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
	g_signal_connect_swapped(menuitem, "activate", G_CALLBACK(
				_on_open_new_tab), ghtml);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	/* open in new window */
	menuitem = gtk_image_menu_item_new_with_mnemonic(
			_("Open in new _window"));
	image = gtk_image_new_from_icon_name("window-new", GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
	g_signal_connect_swapped(menuitem, "activate", G_CALLBACK(
				_on_open_new_window), ghtml);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	/* separator */
	menuitem = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	/* save link as */
	menuitem = gtk_image_menu_item_new_with_mnemonic(_("_Save link as..."));
	image = gtk_image_new_from_stock(GTK_STOCK_SAVE_AS, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
	g_signal_connect_swapped(menuitem, "activate", G_CALLBACK(
				_on_save_link_as), ghtml);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	/* copy link location */
	menuitem = gtk_image_menu_item_new_with_mnemonic(
			_("_Copy link location"));
	image = gtk_image_new_from_stock(GTK_STOCK_COPY, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
	g_signal_connect_swapped(menuitem, "activate", G_CALLBACK(
				_on_copy_link_location), ghtml);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
}

static void _context_menu_media(GHtml * ghtml)
{
	/* FIXME implement */
}

static void _context_menu_position(GtkMenu * menu, gint * x, gint * y,
		gboolean * push, gpointer data)
{
	WebKitHitTestResult * result = data;

	g_object_get(G_OBJECT(result), "x", x, "y", y, NULL);
	*push = TRUE;
}

static void _context_menu_selection(GHtml * ghtml, GtkWidget * menu)
{
	GtkWidget * menuitem;

	/* copy */
	menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_COPY, NULL);
	g_signal_connect_swapped(menuitem, "activate", G_CALLBACK(surfer_copy),
			ghtml->surfer);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	/* select all */
	menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_SELECT_ALL,
			NULL);
	g_signal_connect_swapped(menuitem, "activate", G_CALLBACK(
				surfer_select_all), ghtml->surfer);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
}

static void _context_menu_separator(GtkWidget * menu, gboolean * separator)
{
	GtkWidget * menuitem;

	if(*separator == FALSE)
	{
		*separator = TRUE;
		return;
	}
	menuitem = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	*separator = TRUE;
}


/* on_copy_link_location */
static void _on_copy_link_location(gpointer data)
{
	GHtml * ghtml = data;
	GdkAtom atom;
	GtkClipboard * clipboard;

	/* we can ignore errors */
	atom = gdk_atom_intern("CLIPBOARD", FALSE);
	clipboard = gtk_clipboard_get(atom);
	gtk_clipboard_set_text(clipboard, ghtml->popup_link, -1);
	free(ghtml->popup_link);
	ghtml->popup_link = NULL;
}


/* on_create_web_view */
static WebKitWebView * _on_create_web_view(WebKitWebView * view,
		WebKitNavigationAction * action, gpointer data)
{
	GHtml * ghtml;
	Surfer * surfer;
	GtkWidget * widget;

	if((surfer = surfer_new(NULL)) == NULL)
		return NULL;
	/* FIXME we may want the history to be copied (and then more) */
	if((widget = surfer_get_view(surfer)) == NULL)
	{
		surfer_delete(surfer);
		return NULL;
	}
	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	g_signal_connect(ghtml->view, "ready-to-show", G_CALLBACK(
				_on_web_view_ready), widget);
	return WEBKIT_WEB_VIEW(ghtml->view);
}


static void _on_inspect_page(gpointer data)
{
	GHtml * ghtml = data;

	_ghtml_inspect_url(ghtml->widget, ghtml->popup_link);
	free(ghtml->popup_link);
	ghtml->popup_link = NULL;
}


/* on_open_new_tab */
static void _on_open_new_tab(gpointer data)
{
	GHtml * ghtml = data;

	surfer_open_tab(ghtml->surfer, ghtml->popup_link);
	free(ghtml->popup_link);
	ghtml->popup_link = NULL;
}


/* on_open_new_window */
static void _on_open_new_window(gpointer data)
{
	GHtml * ghtml = data;

	surfer_new(ghtml->popup_link);
	free(ghtml->popup_link);
	ghtml->popup_link = NULL;
}


/* on_save_image_as */
static void _on_save_image_as(gpointer data)
{
	GHtml * ghtml = data;

	/* XXX suggest a filename if possible */
	surfer_download(ghtml->surfer, ghtml->popup_image, NULL);
	free(ghtml->popup_image);
	ghtml->popup_image = NULL;
}


/* on_save_link_as */
static void _on_save_link_as(gpointer data)
{
	GHtml * ghtml = data;

	/* XXX suggest a filename if possible */
	surfer_download(ghtml->surfer, ghtml->popup_link, NULL);
	free(ghtml->popup_link);
	ghtml->popup_link = NULL;
}


/* on_web_view_ready */
static void _on_web_view_ready(WebKitWebView * view, gpointer data)
{
	GHtml * ghtml;
	WebKitWindowProperties * properties;
	gboolean b;
	gint w;
	gint h;

	ghtml = g_object_get_data(G_OBJECT(data), "ghtml");
	properties = webkit_web_view_get_window_properties(
			WEBKIT_WEB_VIEW(view));
	/* FIXME track properties with notify:: instead */
	g_object_get(G_OBJECT(properties), "width", &w, "height", &h, NULL);
	if(w > 0 && h > 0)
		surfer_resize(ghtml->surfer, w, h);
	g_object_get(G_OBJECT(properties), "fullscreen", &b, NULL);
	if(b == TRUE)
		surfer_set_fullscreen(ghtml->surfer, TRUE);
	/* FIXME also applies to location bar? */
#ifndef EMBEDDED
	g_object_get(G_OBJECT(properties), "menubar-visible", &b, NULL);
	surfer_show_menubar(ghtml->surfer, b);
#endif
	g_object_get(G_OBJECT(properties), "toolbar-visible", &b, NULL);
	surfer_show_toolbar(ghtml->surfer, b);
	g_object_get(G_OBJECT(properties), "statusbar-visible", &b, NULL);
	surfer_show_statusbar(ghtml->surfer, b);
	surfer_show_window(ghtml->surfer, TRUE);
}
