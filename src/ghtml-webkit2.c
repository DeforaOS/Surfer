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
#define WITH_INSPECTOR WEBKIT_CHECK_VERSION(1, 0, 3)


/* private */
/* types */
typedef struct _GHtml
{
	Surfer * surfer;
	GtkWidget * widget;
	GtkWidget * view;
#if WITH_INSPECTOR
	GtkWidget * inspector;
#endif
	char * status;
	gboolean ssl;

	/* for popup menus */
	char * popup_image;
	char * popup_link;
} GHtml;


/* prototypes */
/* functions */
/* accessors */
#if WEBKIT_CHECK_VERSION(1, 1, 18)
static void _ghtml_set_favicon(GtkWidget * widget, char const * icon);
#endif
static void _ghtml_set_status(GtkWidget * widget, char const * status);

/* useful */
#ifdef WITH_INSPECTOR
static void _ghtml_inspect_url(GtkWidget * widget, char const * url);
#endif

/* callbacks */
static gboolean _on_console_message(WebKitWebView * view, const gchar * message,
		guint line, const gchar * source, gpointer data);
#if WEBKIT_CHECK_VERSION(1, 10, 0)
static gboolean _on_context_menu(WebKitWebView * view, GtkWidget * menu,
		WebKitHitTestResult * result, gboolean keyboard, gpointer data);
#endif
#if WEBKIT_CHECK_VERSION(1, 10, 0)
static void _on_copy_link_location(gpointer data);
#endif
static WebKitWebView * _on_create_web_view(WebKitWebView * view,
		WebKitWebFrame * frame, gpointer data);
#ifdef WEBKIT_TYPE_DOWNLOAD
static gboolean _on_download_requested(WebKitWebView * view,
		WebKitDownload * download, gpointer data);
#endif
static void _on_hovering_over_link(WebKitWebView * view, const gchar * title,
		const gchar * url, gpointer data);
#if WEBKIT_CHECK_VERSION(1, 1, 18)
static void _on_icon_load(WebKitWebView * view, gchar * icon, gpointer data);
#endif
#if WEBKIT_CHECK_VERSION(1, 10, 0) && defined(WITH_INSPECTOR)
static void _on_inspect_page(gpointer data);
#endif
static void _on_load_committed(WebKitWebView * view, WebKitWebFrame * frame,
		gpointer data);
#if WEBKIT_CHECK_VERSION(1, 1, 6)
static gboolean _on_load_error(WebKitWebView * view, WebKitWebFrame * frame,
		const gchar * uri, GError * error, gpointer data);
#endif
static void _on_load_finished(WebKitWebView * view, WebKitWebFrame * frame,
		gpointer data);
static void _on_load_progress_changed(WebKitWebView * view, gint progress,
		gpointer data);
static void _on_load_started(WebKitWebView * view, WebKitWebFrame * frame,
		gpointer data);
#if WEBKIT_CHECK_VERSION(1, 10, 0)
static void _on_open_new_tab(gpointer data);
static void _on_open_new_window(gpointer data);
static void _on_save_image_as(gpointer data);
static void _on_save_link_as(gpointer data);
#endif
static gboolean _on_script_alert(WebKitWebView * view, WebKitWebFrame * frame,
		const gchar * message, gpointer data);
static gboolean _on_script_confirm(WebKitWebView * view, WebKitWebFrame * frame,
		const gchar * message, gboolean * confirmed, gpointer data);
static gboolean _on_script_prompt(WebKitWebView * view, WebKitWebFrame * frame,
		const gchar * message, const gchar * default_value,
		gchar ** value, gpointer data);
static void _on_status_bar_text_changed(WebKitWebView * view, gchar * arg1,
		gpointer data);
static void _on_title_changed(WebKitWebView * view, WebKitWebFrame * frame,
		const gchar * title, gpointer data);
static gboolean _on_web_view_ready(WebKitWebView * view, gpointer data);


/* public */
/* functions */
/* ghtml_new */
static void _new_init(GHtml * ghtml);

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
#if WITH_INSPECTOR
	ghtml->inspector = NULL;
#endif
	g_object_set_data(G_OBJECT(widget), "ghtml", ghtml);
	/* view */
	g_signal_connect(ghtml->view, "console-message", G_CALLBACK(
				_on_console_message), widget);
#if WEBKIT_CHECK_VERSION(1, 10, 0)
	g_signal_connect(ghtml->view, "context-menu", G_CALLBACK(
				_on_context_menu), widget);
#endif
	g_signal_connect(ghtml->view, "create-web-view", G_CALLBACK(
				_on_create_web_view), widget);
#ifdef WEBKIT_TYPE_DOWNLOAD
	g_signal_connect(ghtml->view, "download-requested", G_CALLBACK(
				_on_download_requested), widget);
#endif
	g_signal_connect(ghtml->view, "hovering-over-link", G_CALLBACK(
				_on_hovering_over_link), widget);
#if WEBKIT_CHECK_VERSION(1, 1, 18)
	g_signal_connect(ghtml->view, "icon-loaded", G_CALLBACK(
				_on_icon_load), widget);
#endif
	g_signal_connect(ghtml->view, "load-committed", G_CALLBACK(
				_on_load_committed), widget);
#if WEBKIT_CHECK_VERSION(1, 1, 6)
	g_signal_connect(ghtml->view, "load-error", G_CALLBACK(
				_on_load_error), widget);
#endif
	g_signal_connect(ghtml->view, "load-finished", G_CALLBACK(
				_on_load_finished), widget);
	g_signal_connect(ghtml->view, "load-progress-changed", G_CALLBACK(
				_on_load_progress_changed), widget);
	g_signal_connect(ghtml->view, "load-started", G_CALLBACK(
				_on_load_started), widget);
	g_signal_connect(ghtml->view, "script-alert", G_CALLBACK(
				_on_script_alert), widget);
	g_signal_connect(ghtml->view, "script-confirm", G_CALLBACK(
				_on_script_confirm), widget);
	g_signal_connect(ghtml->view, "script-prompt", G_CALLBACK(
				_on_script_prompt), widget);
	g_signal_connect(ghtml->view, "status-bar-text-changed",
			G_CALLBACK(_on_status_bar_text_changed), widget);
	g_signal_connect(ghtml->view, "title-changed", G_CALLBACK(
				_on_title_changed), widget);
	/* scrolled window */
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(widget), ghtml->view);
	_new_init(ghtml);
	return widget;
}

static void _new_init(GHtml * ghtml)
{
	/* FIXME implement fonts (appearance, fonts) */
	static int initialized = 0;
#if WEBKIT_CHECK_VERSION(1, 1, 0)
	SoupSession * session;
# if WEBKIT_CHECK_VERSION(1, 3, 5) && defined(EMBEDDED)
	WebKitWebSettings * settings;
# endif
	char const * cacerts[] =
	{
		"/etc/pki/tls/certs/ca-bundle.crt",
		"/etc/ssl/certs/ca-certificates.crt",
		"/etc/openssl/certs/ca-certificates.crt",
		PREFIX "/etc/ssl/certs/ca-certificates.crt",
		PREFIX "/etc/openssl/certs/ca-certificates.crt"
	};
	size_t i;
#endif

	if(initialized++ != 0)
	{
		initialized = 1;
		return;
	}
#if WEBKIT_CHECK_VERSION(1, 1, 0)
	session = webkit_get_default_session();
# if WEBKIT_CHECK_VERSION(1, 3, 5) && defined(EMBEDDED)
	settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(ghtml->view));
	g_object_set(settings, "enable-frame-flattening", TRUE, NULL);
# endif
	for(i = 0; i < sizeof(cacerts) / sizeof(*cacerts); i++)
		if(access(cacerts[i], R_OK) == 0)
		{
			g_object_set(session, "ssl-ca-file", cacerts[i],
					"ssl-strict", FALSE, NULL);
			ghtml->ssl = TRUE;
			return;
		}
#endif
	surfer_warning(ghtml->surfer, _("Could not load certificate bundle:\n"
			"SSL certificates will not be verified."));
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
	GHtml * ghtml;
	WebKitWebFrame * frame;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
#if WEBKIT_CHECK_VERSION(1, 8, 0)
	if((frame = webkit_web_view_get_main_frame(
					WEBKIT_WEB_VIEW(ghtml->view))) != NULL
			&& webkit_web_frame_get_uri(frame) != NULL)
		return webkit_web_view_try_get_favicon_pixbuf(
				WEBKIT_WEB_VIEW(ghtml->view), 16, 16);
#else
	/* FIXME implement */
#endif
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
	WebKitWebFrame * frame;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	frame = webkit_web_view_get_main_frame(WEBKIT_WEB_VIEW(ghtml->view));
	return webkit_web_frame_get_uri(frame);
}


/* ghtml_get_progress */
gdouble ghtml_get_progress(GtkWidget * widget)
{
	gdouble ret = -1.0;
#if WEBKIT_CHECK_VERSION(1, 1, 0) /* XXX may not be accurate */
	GHtml * ghtml;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	ret = webkit_web_view_get_progress(WEBKIT_WEB_VIEW(ghtml->view));
	if(ret == 0.0)
		ret = -1.0;
#endif
	return ret;
}


/* ghtml_get_security */
SurferSecurity ghtml_get_security(GtkWidget * widget)
{
	SurferSecurity security = SS_NONE;
#if WEBKIT_CHECK_VERSION(1, 1, 0)
	GHtml * ghtml;
	WebKitWebFrame * frame;
	char const * location;
	WebKitWebDataSource *source;
	WebKitNetworkRequest *request;
	SoupMessage * message;
#endif

#if WEBKIT_CHECK_VERSION(1, 1, 0)
	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	frame = webkit_web_view_get_main_frame(WEBKIT_WEB_VIEW(ghtml->view));
	if((location = webkit_web_frame_get_uri(frame)) != NULL
			&& strncmp(location, "https://", 8) == 0)
	{
		security = SS_UNTRUSTED;
		source = webkit_web_frame_get_data_source(frame);
		request = webkit_web_data_source_get_request(source);
		message = webkit_network_request_get_message(request);
		if(ghtml->ssl == TRUE && message != NULL
				&& soup_message_get_flags(message)
				& SOUP_MESSAGE_CERTIFICATE_TRUSTED)
			security = SS_TRUSTED;
	}
#endif
	return security;
}


/* ghtml_get_source */
char const * ghtml_get_source(GtkWidget * widget)
{
#if WEBKIT_CHECK_VERSION(1, 1, 0)
	GHtml * ghtml;
	WebKitWebFrame * frame;
	WebKitWebDataSource * source;
	GString * str;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	frame = webkit_web_view_get_main_frame(WEBKIT_WEB_VIEW(ghtml->view));
	source = webkit_web_frame_get_data_source(frame);
	if((str = webkit_web_data_source_get_data(source)) == NULL)
		return NULL;
	return str->str;
#else
	return NULL;
#endif
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
	WebKitWebFrame * frame;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	frame = webkit_web_view_get_main_frame(WEBKIT_WEB_VIEW(ghtml->view));
	return webkit_web_frame_get_title(frame);
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
	WebKitWebSettings * settings;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(ghtml->view));
	g_object_set(settings, "enable-scripts", enable, NULL);
	return 0;
}


/* ghtml_set_proxy */
#if WEBKIT_CHECK_VERSION(1, 1, 0)
static SoupURI * _set_proxy_address(struct addrinfo * ai);
#endif

int ghtml_set_proxy(GtkWidget * widget, SurferProxyType type, char const * http,
		uint16_t http_port)
{
#if WEBKIT_CHECK_VERSION(1, 1, 0)
	SoupSession * session;
	char buf[32];
	struct addrinfo hints;
	struct addrinfo * ai;
	struct addrinfo * aip;
	int res;
	SoupURI * uri = NULL;

	session = webkit_get_default_session();
	if(type == SPT_HTTP && http != NULL && strlen(http) > 0)
	{
		snprintf(buf, sizeof(buf), "%hu", http_port);
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_NUMERICSERV;
		if((res = getaddrinfo(http, buf, &hints, &ai)) != 0)
			return -error_set_code(1, "%s: %s", http, gai_strerror(
						res));
		for(aip = ai; aip != NULL; aip = aip->ai_next)
			if((uri = _set_proxy_address(aip)) != NULL)
				break;
		freeaddrinfo(ai);
		if(uri == NULL)
			return -error_set_code(1, "%s: %s", http,
					"No suitable address found for proxy");
	}
	g_object_set(session, "proxy-uri", uri, NULL);
	return 0;
#else
	/* FIXME really implement */
	return -error_set_code(1, "%s", strerror(ENOSYS));
#endif
}

#if WEBKIT_CHECK_VERSION(1, 1, 0)
static SoupURI * _set_proxy_address(struct addrinfo * ai)
{
	char buf[128];
	char buf2[128];
	struct sockaddr_in * sin;
	struct sockaddr_in6 * sin6;

	switch(ai->ai_family)
	{
		case AF_INET:
			sin = (struct sockaddr_in *)ai->ai_addr;
			if(inet_ntop(ai->ai_family, &sin->sin_addr, buf,
						sizeof(buf)) == NULL)
				return NULL;
			snprintf(buf2, sizeof(buf2), "http://%s:%hu/", buf,
					ntohs(sin->sin_port));
			return soup_uri_new(buf2);
		case AF_INET6:
			sin6 = (struct sockaddr_in6 *)ai->ai_addr;
			if(inet_ntop(ai->ai_family, &sin6->sin6_addr, buf,
						sizeof(buf)) == NULL)
				return NULL;
			snprintf(buf2, sizeof(buf2), "http://[%s]:%hu/", buf,
					ntohs(sin6->sin6_port));
			return soup_uri_new(buf2);
		default:
			return NULL;
	}
}
#endif


/* ghtml_set_user_agent */
int ghtml_set_user_agent(GtkWidget * ghtml, char const * user_agent)
{
#if WEBKIT_CHECK_VERSION(1, 1, 0)
	SoupSession * session;

	session = webkit_get_default_session();
	g_object_set(session, "user-agent",
			(user_agent == NULL || strlen(user_agent) == 0)
			? NULL : user_agent, NULL);
	return 0;
#else
	if(user_agent == NULL || strlen(user_agent) == 0)
		return 0;
	return -error_set_code(1, "%s", strerror(ENOSYS));
#endif
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
	webkit_web_view_copy_clipboard(WEBKIT_WEB_VIEW(ghtml->view));
}


/* ghtml_cut */
void ghtml_cut(GtkWidget * widget)
{
	GHtml * ghtml;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	webkit_web_view_cut_clipboard(WEBKIT_WEB_VIEW(ghtml->view));
}


/* ghtml_execute */
void ghtml_execute(GtkWidget * widget, char const * code)
{
	GHtml * ghtml;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	webkit_web_view_execute_script(WEBKIT_WEB_VIEW(ghtml->view), code);
}


/* ghtml_find */
gboolean ghtml_find(GtkWidget * widget, char const * text, gboolean sensitive,
		gboolean backwards, gboolean wrap)
{
	GHtml * ghtml;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	return webkit_web_view_search_text(WEBKIT_WEB_VIEW(ghtml->view), text,
			sensitive, !backwards, wrap);
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
	if(strcmp("about:blank", url) == 0)
		webkit_web_view_load_string(WEBKIT_WEB_VIEW(ghtml->view), "",
				NULL, NULL, url);
	else if(strncmp("about:", url, 6) == 0)
		webkit_web_view_load_string(WEBKIT_WEB_VIEW(ghtml->view),
				about, NULL, NULL, url);
	else
	{
#if WEBKIT_CHECK_VERSION(1, 1, 1)
		webkit_web_view_load_uri(WEBKIT_WEB_VIEW(ghtml->view), url);
#else
		webkit_web_view_open(WEBKIT_WEB_VIEW(ghtml->view), url);
#endif
	}
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
	webkit_web_view_paste_clipboard(WEBKIT_WEB_VIEW(ghtml->view));
}


/* ghtml_print */
void ghtml_print(GtkWidget * widget)
{
#if WEBKIT_CHECK_VERSION(1, 1, 0) /* XXX may not be accurate */
	GHtml * ghtml;
	WebKitWebFrame * frame;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	frame = webkit_web_view_get_main_frame(WEBKIT_WEB_VIEW(ghtml->view));
	webkit_web_frame_print(frame);
#endif
}


/* ghtml_redo */
void ghtml_redo(GtkWidget * widget)
{
#if WEBKIT_CHECK_VERSION(1, 1, 14)
	GHtml * ghtml;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	webkit_web_view_redo(WEBKIT_WEB_VIEW(ghtml->view));
#endif
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
#if WEBKIT_CHECK_VERSION(1, 0, 3)
	webkit_web_view_reload_bypass_cache(WEBKIT_WEB_VIEW(ghtml->view));
#else
	webkit_web_view_reload(WEBKIT_WEB_VIEW(ghtml->view));
#endif
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
	webkit_web_view_select_all(WEBKIT_WEB_VIEW(ghtml->view));
}


/* ghtml_undo */
void ghtml_undo(GtkWidget * widget)
{
#if WEBKIT_CHECK_VERSION(1, 1, 14)
	GHtml * ghtml;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	webkit_web_view_undo(WEBKIT_WEB_VIEW(ghtml->view));
#endif
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

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	webkit_web_view_zoom_in(WEBKIT_WEB_VIEW(ghtml->view));
}


/* ghtml_zoom_out */
void ghtml_zoom_out(GtkWidget * widget)
{
	GHtml * ghtml;

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	webkit_web_view_zoom_out(WEBKIT_WEB_VIEW(ghtml->view));
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
#if WEBKIT_CHECK_VERSION(1, 1, 18)
/* ghtml_set_favicon */
static void _ghtml_set_favicon(GtkWidget * widget, char const * icon)
{
	GHtml * ghtml;
	GdkPixbuf * pixbuf = NULL;
# if WEBKIT_CHECK_VERSION(1, 8, 0)
	WebKitWebFrame * frame;
# endif

	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
# if WEBKIT_CHECK_VERSION(1, 8, 0)
	if((frame = webkit_web_view_get_main_frame(
					WEBKIT_WEB_VIEW(ghtml->view))) != NULL
			&& webkit_web_frame_get_uri(frame) != NULL)
		pixbuf = webkit_web_view_try_get_favicon_pixbuf(
				WEBKIT_WEB_VIEW(ghtml->view), 16, 16);
# else
	/* FIXME implement */
# endif
	surfer_set_favicon(ghtml->surfer, pixbuf);
}
#endif


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
#ifdef WITH_INSPECTOR
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
	WebKitWebSettings * settings;
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
	g_signal_connect(view, "console-message", G_CALLBACK(
				_on_console_message), ghtml->widget);
	g_signal_connect_swapped(view, "web-view-ready", G_CALLBACK(
				gtk_widget_show_all), ghtml->inspector);
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
#endif


/* callbacks */
/* on_console_message */
static gboolean _on_console_message(WebKitWebView * view, const gchar * message,
		guint line, const gchar * source, gpointer data)
{
	GHtml * ghtml;

	ghtml = g_object_get_data(G_OBJECT(data), "ghtml");
	surfer_console_message(ghtml->surfer, message, source, line);
	return TRUE;
}


#if WEBKIT_CHECK_VERSION(1, 10, 0)
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

static gboolean _on_context_menu(WebKitWebView * view, GtkWidget * menu,
		WebKitHitTestResult * result, gboolean keyboard, gpointer data)
{
	GtkWidget * widget = data;
	GHtml * ghtml;
	WebKitHitTestResultContext context;
	gboolean separator = FALSE;

	/* FIXME implement every callback */
	ghtml = g_object_get_data(G_OBJECT(widget), "ghtml");
	menu = gtk_menu_new();
	g_object_get(G_OBJECT(result), "context", &context, NULL);
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
	if(keyboard)
		/* XXX seems to be buggy */
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL,
				_context_menu_position, result, -1,
				gtk_get_current_event_time());
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
#ifdef WITH_INSPECTOR
	/* inspect */
	menuitem = gtk_image_menu_item_new_with_mnemonic(
			_("_Inspect this page"));
	g_signal_connect_swapped(menuitem, "activate", G_CALLBACK(
				_on_inspect_page), ghtml);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
#endif
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
#endif


#if WEBKIT_CHECK_VERSION(1, 10, 0)
/* on_copy_link_location */
static void _on_copy_link_location(gpointer data)
{
	GHtml * ghtml = data;
	GdkAtom atom;
	GtkClipboard * clipboard;

	/* we can ignore errors */
	atom = gdk_atom_intern ("CLIPBOARD", FALSE);
	clipboard = gtk_clipboard_get(atom);
	gtk_clipboard_set_text(clipboard, ghtml->popup_link, -1);
	free(ghtml->popup_link);
	ghtml->popup_link = NULL;
}
#endif


/* on_create_web_view */
static WebKitWebView * _on_create_web_view(WebKitWebView * view,
		WebKitWebFrame * frame, gpointer data)
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
	g_signal_connect(ghtml->view, "web-view-ready", G_CALLBACK(
				_on_web_view_ready), widget);
	return WEBKIT_WEB_VIEW(ghtml->view);
}


#ifdef WEBKIT_TYPE_DOWNLOAD
/* on_download_requested */
static gboolean _on_download_requested(WebKitWebView * view,
		WebKitDownload * download, gpointer data)
{
	GHtml * ghtml;
	char const * url;
	char const * suggested;

	ghtml = g_object_get_data(G_OBJECT(data), "ghtml");
	url = webkit_download_get_uri(download);
	suggested = webkit_download_get_suggested_filename(download);
	surfer_download(ghtml->surfer, url, suggested);
	webkit_download_cancel(download);
	return FALSE;
}
#endif


/* on_hovering_over_link */
static void _on_hovering_over_link(WebKitWebView * view, const gchar * title,
		const gchar * url, gpointer data)
{
	GtkWidget * widget = data;

	_ghtml_set_status(widget, url);
}


#if WEBKIT_CHECK_VERSION(1, 1, 18)
static void _on_icon_load(WebKitWebView * view, gchar * icon, gpointer data)
{
	GtkWidget * widget = data;

	_ghtml_set_favicon(widget, icon);
}
#endif


#if WEBKIT_CHECK_VERSION(1, 10, 0) && defined(WITH_INSPECTOR)
static void _on_inspect_page(gpointer data)
{
	GHtml * ghtml = data;

	_ghtml_inspect_url(ghtml->widget, ghtml->popup_link);
	free(ghtml->popup_link);
	ghtml->popup_link = NULL;
}
#endif


/* on_load_committed */
static void _on_load_committed(WebKitWebView * view, WebKitWebFrame * frame,
		gpointer data)
{
	GHtml * ghtml;
	char const * location;

	ghtml = g_object_get_data(G_OBJECT(data), "ghtml");
	if(frame == webkit_web_view_get_main_frame(view)
			&& (location = webkit_web_frame_get_uri(frame)) != NULL)
		surfer_set_location(ghtml->surfer, location);
	surfer_set_security(ghtml->surfer, ghtml_get_security(ghtml->widget));
}


/* on_load_error */
#if WEBKIT_CHECK_VERSION(1, 1, 6)
static gboolean _on_load_error(WebKitWebView * view, WebKitWebFrame * frame,
		const gchar * uri, GError * error, gpointer data)
{
	GHtml * ghtml;
# ifdef WEBKIT_POLICY_ERROR
	char const * suggested;
# endif

	ghtml = g_object_get_data(G_OBJECT(data), "ghtml");
	if(error == NULL)
		return surfer_error(ghtml->surfer, _("Unknown error"), TRUE);
# ifdef WEBKIT_NETWORK_ERROR
	if(error->domain == WEBKIT_NETWORK_ERROR
			&& error->code == WEBKIT_NETWORK_ERROR_CANCELLED)
		return TRUE; /* ignored if the user cancelled it */
# endif
# ifdef WEBKIT_POLICY_ERROR
	if(error->domain == WEBKIT_POLICY_ERROR
			&& error->code == WEBKIT_POLICY_ERROR_FRAME_LOAD_INTERRUPTED_BY_POLICY_CHANGE)
	{
		/* FIXME propose to download or cancel instead */
		if((suggested = strrchr(uri, '/')) != NULL)
			suggested++;
		surfer_download(ghtml->surfer, uri, suggested);
		return TRUE;
	}
# endif
	return surfer_error(ghtml->surfer, error->message, TRUE);
}
#endif


/* on_load_finished */
static void _on_load_finished(WebKitWebView * view, WebKitWebFrame * arg1,
			gpointer data)
{
	GHtml * ghtml;

	ghtml = g_object_get_data(G_OBJECT(data), "ghtml");
	surfer_set_progress(ghtml->surfer, -1.0);
	_ghtml_set_status(ghtml->widget, NULL);
}


/* on_load_progress_changed */
static void _on_load_progress_changed(WebKitWebView * view, gint progress,
		gpointer data)
{
	GHtml * ghtml;
	gdouble fraction = progress;

	ghtml = g_object_get_data(G_OBJECT(data), "ghtml");
	surfer_set_progress(ghtml->surfer, fraction / 100);
	_ghtml_set_status(ghtml->widget, _("Downloading..."));
}


/* on_load_started */
static void _on_load_started(WebKitWebView * view, WebKitWebFrame * frame,
		gpointer data)
{
	GHtml * ghtml;

	ghtml = g_object_get_data(G_OBJECT(data), "ghtml");
	surfer_set_progress(ghtml->surfer, 0.00);
	_ghtml_set_status(ghtml->widget, _("Downloading..."));
}


#if WEBKIT_CHECK_VERSION(1, 10, 0)
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
#endif


/* on_script_alert */
static gboolean _on_script_alert(WebKitWebView * view, WebKitWebFrame * frame,
		const gchar * message, gpointer data)
{
	GHtml * ghtml;

	ghtml = g_object_get_data(G_OBJECT(data), "ghtml");
	surfer_warning(ghtml->surfer, message);
	return TRUE;
}


static gboolean _on_script_confirm(WebKitWebView * view, WebKitWebFrame * frame,
		const gchar * message, gboolean * confirmed, gpointer data)
{
	GHtml * ghtml;

	ghtml = g_object_get_data(G_OBJECT(data), "ghtml");
	if(surfer_confirm(ghtml->surfer, message, confirmed) != 0)
		*confirmed = FALSE;
	return TRUE;
}

static gboolean _on_script_prompt(WebKitWebView * view, WebKitWebFrame * frame,
		const gchar * message, const gchar * default_value,
		gchar ** value, gpointer data)
{
	GHtml * ghtml;

	ghtml = g_object_get_data(G_OBJECT(data), "ghtml");
	if(surfer_prompt(ghtml->surfer, message, default_value, value) == 0)
		return TRUE;
	*value = NULL;
	return TRUE;
}


static void _on_status_bar_text_changed(WebKitWebView * view, gchar * arg1,
		gpointer data)
{
	GtkWidget * widget = data;

	if(strlen(arg1) == 0)
		return;
	_ghtml_set_status(widget, arg1);
}


static void _on_title_changed(WebKitWebView * view, WebKitWebFrame * frame,
		const gchar * title, gpointer data)
{
	GHtml * ghtml;

	ghtml = g_object_get_data(G_OBJECT(data), "ghtml");
	surfer_set_title(ghtml->surfer, title);
}


#if WEBKIT_CHECK_VERSION(1, 0, 3)
static gboolean _on_web_view_ready(WebKitWebView * view, gpointer data)
{
	GHtml * ghtml;
	WebKitWebWindowFeatures * features;
	gboolean b;
	gint w;
	gint h;

	ghtml = g_object_get_data(G_OBJECT(data), "ghtml");
	features = webkit_web_view_get_window_features(WEBKIT_WEB_VIEW(view));
	/* FIXME track properties with notify:: instead */
	g_object_get(G_OBJECT(features), "width", &w, "height", &h, NULL);
	if(w > 0 && h > 0)
		surfer_resize(ghtml->surfer, w, h);
	g_object_get(G_OBJECT(features), "fullscreen", &b, NULL);
	if(b == TRUE)
		surfer_set_fullscreen(ghtml->surfer, TRUE);
	/* FIXME also applies to location bar? */
# ifndef EMBEDDED
	g_object_get(G_OBJECT(features), "menubar-visible", &b, NULL);
	surfer_show_menubar(ghtml->surfer, b);
# endif
	g_object_get(G_OBJECT(features), "toolbar-visible", &b, NULL);
	surfer_show_toolbar(ghtml->surfer, b);
	g_object_get(G_OBJECT(features), "statusbar-visible", &b, NULL);
	surfer_show_statusbar(ghtml->surfer, b);
	surfer_show_window(ghtml->surfer, TRUE);
	return FALSE;
}
#else /* WebKitWebWindowFeatures is not available */
static gboolean _on_web_view_ready(WebKitWebView * view, gpointer data)
{
	GHtml * ghtml;

	ghtml = g_object_get_data(G_OBJECT(data), "ghtml");
	surfer_show_window(ghtml->surfer, TRUE);
	return FALSE;
}
#endif
