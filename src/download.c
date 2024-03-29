/* $Id$ */
/* Copyright (c) 2009-2022 Pierre Pronchery <khorben@defora.org> */
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
 * - let the checkbox to close window be a global option
 * - also use the proxy settings
 * - use the "Last modified" header (if available?) to futimes() the file */



#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <libgen.h>
#include <errno.h>
#include <locale.h>
#include <libintl.h>
#include <gtk/gtk.h>
#if GTK_CHECK_VERSION(3, 0, 0)
# include <gtk/gtkx.h>
#endif
#include <System.h>
#if defined(WITH_WEBKIT)
# include <sys/types.h>
# include <sys/socket.h>
# include <netdb.h>
# include <arpa/inet.h>
# include <webkit/webkit.h>
#elif defined(WITH_WEBKIT2)
# include <sys/types.h>
# include <sys/socket.h>
# include <netdb.h>
# include <arpa/inet.h>
# include <webkit2/webkit2.h>
#else
# define GNET_EXPERIMENTAL
# include <gnet.h>
#endif
#include "download.h"
#include "../config.h"
#include "common/url.c"
#define _(string) gettext(string)
#define N_(string) (string)

/* constants */
#ifndef PROGNAME_BROWSER
# define PROGNAME_BROWSER	"browser"
#endif
#ifndef PROGNAME_DOWNLOAD
# define PROGNAME_DOWNLOAD	"download"
#endif

#ifndef PREFIX
# define PREFIX			"/usr/local"
#endif
#ifndef DATADIR
# define DATADIR		PREFIX "/share"
#endif
#ifndef LOCALEDIR
# define LOCALEDIR		DATADIR "/locale"
#endif


/* Download */
/* private */
/* types */
struct _Download
{
	DownloadPrefs prefs;
	char * url;

	struct timeval tv;

#if defined(WITH_WEBKIT) || defined(WITH_WEBKIT2)
	WebKitDownload * conn;
#else
	FILE * fp;
	GConnHttp * conn;
#endif
	guint64 content_length;
	guint64 data_received;

	/* widgets */
	GtkWidget * window;
	GtkWidget * address;
	GtkWidget * filename;
	GtkWidget * status;
	GtkWidget * received;
	GtkWidget * remaining;
	GtkWidget * progress;
	GtkWidget * check;
	GtkWidget * browse;
	GtkWidget * cancel;

	guint timeout;
	int pulse;
};


/* variables */
#ifdef WITH_MAIN
static unsigned int _download_cnt = 0;
#endif


/* prototypes */
static int _download_error(Download * download, char const * message, int ret);

static int _download_set_proxy(Download * download, char const * http,
		uint16_t http_port);

static void _download_refresh(Download * download);
#if !defined(WITH_WEBKIT) && !defined(WITH_WEBKIT2)
static int _download_write(Download * download);
#endif

/* callbacks */
static void _download_on_browse(gpointer data);
static void _download_on_cancel(gpointer data);
static gboolean _download_on_closex(gpointer data);
static void _download_on_embedded(gpointer data);

#if !defined(WITH_WEBKIT) && !defined(WITH_WEBKIT2)
static void _download_on_http(GConnHttp * conn, GConnHttpEvent * event,
		gpointer data);
#endif
static gboolean _download_on_idle(gpointer data);
static gboolean _download_on_timeout(gpointer data);


/* public */
/* functions */
/* download_new */
static void _download_label(GtkWidget * vbox, PangoFontDescription * bold,
		GtkSizeGroup * left, char const * label, GtkWidget ** widget,
		char const * text);

Download * download_new(DownloadPrefs * prefs, char const * url)
{
	Download * download;
	char * p;
	char buf[256];
	GtkWidget * vbox;
	GtkWidget * hbox;
	GtkSizeGroup * left;
	GtkWidget * widget;
	PangoFontDescription * bold;
	unsigned long id;

	/* verify arguments */
	if(prefs == NULL || url == NULL)
	{
		errno = EINVAL;
		_download_error(NULL, NULL, 1);
		return NULL;
	}
	if((download = malloc(sizeof(*download))) == NULL)
	{
		_download_error(NULL, "malloc", 1);
		return NULL;
	}
	/* initialize structure */
	download->prefs.output = (prefs->output != NULL) ? strdup(prefs->output)
		: NULL;
	download->prefs.user_agent = (prefs->user_agent != NULL)
		? strdup(prefs->user_agent) : NULL;
	if((p = _ghtml_make_url(NULL, url)) != NULL)
		url = p;
	download->url = strdup(url);
	free(p);
	if(download->url != NULL && prefs->output == NULL)
		download->prefs.output = strdup(basename(download->url));
	download->conn = NULL;
	download->data_received = 0;
	download->content_length = 0;
	download->timeout = 0;
	download->pulse = 0;
	/* verify initialization */
	if((prefs->output != NULL && download->prefs.output == NULL)
			|| (prefs->user_agent != NULL
				&& download->prefs.user_agent == NULL)
			|| download->url == NULL
			|| gettimeofday(&download->tv, NULL) != 0)
	{
		_download_error(NULL, "gettimeofday", 1);
		download_delete(download);
		return NULL;
	}
	/* window */
	if(prefs->embedded == 0)
	{
		download->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		snprintf(buf, sizeof(buf), "%s %s", _("Download"),
				download->url);
#if GTK_CHECK_VERSION(2, 6, 0)
		gtk_window_set_icon_name(GTK_WINDOW(download->window),
				"stock_download");
#endif
		gtk_window_set_resizable(GTK_WINDOW(download->window), FALSE);
		gtk_window_set_title(GTK_WINDOW(download->window), buf);
		g_signal_connect_swapped(download->window, "delete-event",
				G_CALLBACK(_download_on_closex), download);
	}
	else
	{
		download->window = gtk_plug_new(0);
		g_signal_connect_swapped(download->window, "embedded",
				G_CALLBACK(_download_on_embedded), download);
	}
#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
#else
	vbox = gtk_vbox_new(FALSE, 2);
#endif
	bold = pango_font_description_new();
	pango_font_description_set_weight(bold, PANGO_WEIGHT_BOLD);
	left = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	/* address */
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	widget = gtk_label_new(_("Address: "));
#if GTK_CHECK_VERSION(3, 0, 0)
	gtk_widget_override_font(widget, bold);
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_widget_modify_font(widget, bold);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(left, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	download->address = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(download->address), download->url);
	gtk_editable_set_editable(GTK_EDITABLE(download->address), FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), download->address, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	/* labels */
	_download_label(vbox, bold, left, _("File: "), &download->filename,
			download->prefs.output);
	_download_label(vbox, bold, left, _("Status: "), &download->status,
			_("Resolving..."));
	_download_label(vbox, bold, left, _("Received: "), &download->received,
			_("0.0 kB"));
	_download_label(vbox, bold, left, _("Remaining: "),
			&download->remaining, _("Unknown"));
	/* progress bar */
	download->progress = gtk_progress_bar_new();
	gtk_box_pack_start(GTK_BOX(vbox), download->progress, FALSE, FALSE, 0);
	/* checkbox */
	download->check = gtk_check_button_new_with_label(
			_("Close window when the download is complete"));
	gtk_box_pack_start(GTK_BOX(vbox), download->check, FALSE, FALSE, 0);
	/* button */
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	download->cancel = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	g_signal_connect_swapped(download->cancel, "clicked", G_CALLBACK(
				_download_on_cancel), download);
	gtk_box_pack_end(GTK_BOX(hbox), download->cancel, FALSE, TRUE, 0);
	download->browse = gtk_button_new_with_mnemonic("_Open folder");
	gtk_widget_set_no_show_all(download->browse, TRUE);
	widget = gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(download->browse), widget);
	g_signal_connect_swapped(download->browse, "clicked", G_CALLBACK(
				_download_on_browse), download);
	gtk_box_pack_end(GTK_BOX(hbox), download->browse, FALSE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(download->window), 4);
	gtk_container_add(GTK_CONTAINER(download->window), vbox);
	download->timeout = g_idle_add(_download_on_idle, download);
	_download_refresh(download);
	gtk_widget_show_all(vbox);
	if(prefs->embedded == 0)
		gtk_widget_show(download->window);
	else
	{
		id = gtk_plug_get_id(GTK_PLUG(download->window));
		printf("%lu\n", id);
		fclose(stdout);
	}
	_download_cnt++;
	return download;
}

static void _download_label(GtkWidget * vbox, PangoFontDescription * bold,
		GtkSizeGroup * left, char const * label, GtkWidget ** widget,
		char const * text)
{
	GtkWidget * hbox;

#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	*widget = gtk_label_new(label);
#if GTK_CHECK_VERSION(3, 0, 0)
	gtk_widget_override_font(*widget, bold);
	g_object_set(*widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_widget_modify_font(*widget, bold);
	gtk_misc_set_alignment(GTK_MISC(*widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(left, *widget);
	gtk_box_pack_start(GTK_BOX(hbox), *widget, FALSE, TRUE, 0);
	*widget = gtk_label_new(text);
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(*widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(*widget), 0.0, 0.5);
#endif
	gtk_box_pack_start(GTK_BOX(hbox), *widget, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
}


/* download_delete */
void download_delete(Download * download)
{
	if(download->timeout != 0)
		g_source_remove(download->timeout);
#if defined(WITH_WEBKIT) || defined(WITH_WEBKIT2)
	if(download->conn != NULL)
	{
		webkit_download_cancel(download->conn);
		if(unlink(download->prefs.output) != 0)
			_download_error(download, download->prefs.output, 1);
		/* XXX should also unlink the (temporary) output file */
	}
#else
	if(download->conn != NULL)
		gnet_conn_http_delete(download->conn);
	if(download->fp != NULL)
	{
		if(fclose(download->fp) != 0)
			_download_error(download, download->prefs.output, 1);
		if(unlink(download->prefs.output) != 0)
			_download_error(download, download->prefs.output, 1);
	}
#endif
	free(download->url);
	free(download->prefs.user_agent);
	free(download->prefs.output);
	gtk_widget_destroy(download->window);
	free(download);
	if(--_download_cnt == 0)
		gtk_main_quit();
}


/* accessors */
void download_set_close(Download * download, gboolean close)
{
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(download->check), close);
}


/* useful */
/* download_cancel */
int download_cancel(Download * download)
{
	download_delete(download);
	return 0;
}


/* private */
/* functions */
/* download_error */
static int _download_error(Download * download, char const * message, int ret)
{
	GtkWidget * dialog;

	if(ret < 0)
	{
		fputs(PROGNAME_DOWNLOAD ": ", stderr);
		perror(message);
		return -ret;
	}
	dialog = gtk_message_dialog_new((download != NULL)
			? GTK_WINDOW(download->window) : NULL,
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
			GTK_BUTTONS_CLOSE,
#if GTK_CHECK_VERSION(2, 6, 0)
			"%s", _("Error"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
#endif
			"%s: %s",
			message, strerror(errno));
	gtk_window_set_title(GTK_WINDOW(dialog), _("Error"));
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	return ret;
}


/* download_set_proxy */
#if defined(WITH_WEBKIT) || defined(WITH_WEBKIT2)
# if WEBKIT_CHECK_VERSION(1, 1, 0)
static SoupURI * _set_proxy_address(struct addrinfo * ai);
# endif
#endif

static int _download_set_proxy(Download * download, char const * http,
		uint16_t http_port)
{
#if defined(WITH_WEBKIT) || defined(WITH_WEBKIT2)
# if WEBKIT_CHECK_VERSION(1, 1, 0)
	SoupSession * session;
	struct addrinfo hints;
	struct addrinfo * ai;
	struct addrinfo * aip;
	char buf[6];
	int res;
	SoupURI * uri = NULL;

	session = webkit_get_default_session();
	if(http != NULL && strlen(http) > 0)
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
# else
	/* FIXME really implement */
	return -error_set_code(1, "%s", strerror(ENOSYS));
# endif
#else
	/* FIXME really implement */
	return -error_set_code(1, "%s", strerror(ENOSYS));
#endif
}

#if defined(WITH_WEBKIT) || defined(WITH_WEBKIT2)
# if WEBKIT_CHECK_VERSION(1, 1, 0)
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
# endif
#endif


/* download_refresh */
static void _refresh_unit(guint64 total, double * fraction, char const ** unit,
		double * current);
static void _refresh_remaining(Download * download, guint64 rate);

static void _download_refresh(Download * download)
{
	char buf[256];
	struct timeval tv;
	double current_fraction;
	guint64 rate = 0;
	double rate_fraction = 0.0;
	char const * rate_unit = N_("kB");
	double total_fraction;
	char const * total_unit = N_("kB");

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() %lu/%lu\n", __func__,
			download->data_received, download->content_length);
#endif
	/* XXX should check gettimeofday() return value explicitly */
	if(download->data_received > 0 && gettimeofday(&tv, NULL) == 0)
	{
		if((tv.tv_sec = tv.tv_sec - download->tv.tv_sec) < 0)
			tv.tv_sec = 0;
		if((tv.tv_usec = tv.tv_usec - download->tv.tv_usec) < 0)
		{
			tv.tv_sec--;
			tv.tv_usec += 1000000;
		}
		rate = (download->data_received * 1024)
			/ ((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
		_refresh_unit(rate, &rate_fraction, &rate_unit, NULL);
	}
	if(download->content_length == 0)
	{
		/* the total size is not known */
		_refresh_unit(download->data_received, &total_fraction,
				&total_unit, NULL);
		snprintf(buf, sizeof(buf), _("%.1f %s (%.1f %s/s)"),
				total_fraction, total_unit, rate_fraction,
				rate_unit);
		gtk_label_set_text(GTK_LABEL(download->received), buf);
		snprintf(buf, sizeof(buf), " ");
		/* pulse the progress bar if any data was received */
		if(download->pulse != 0)
		{
			gtk_progress_bar_pulse(GTK_PROGRESS_BAR(
						download->progress));
			download->pulse = 0;
		}
	}
	else
	{
		/* the total size is known */
		current_fraction = download->data_received;
		_refresh_unit(download->content_length, &total_fraction,
				&total_unit, &current_fraction);
		snprintf(buf, sizeof(buf), _("%.1f of %.1f %s (%.1f %s/s)"),
				current_fraction, total_fraction, _(total_unit),
				rate_fraction, _(rate_unit));
		gtk_label_set_text(GTK_LABEL(download->received), buf);
		total_fraction = download->data_received;
		total_fraction /= download->content_length;
		snprintf(buf, sizeof(buf), "%.1f%%", total_fraction * 100);
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(
					download->progress), total_fraction);
#if GTK_CHECK_VERSION(3, 0, 0)
		gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(
					download->progress), TRUE);
#endif
		_refresh_remaining(download, rate);
	}
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(download->progress), buf);
}

static void _refresh_remaining(Download * download, guint64 rate)
{
	char buf[32];
	guint64 remaining;
	struct tm tm;

	if(rate == 0)
		return;
	remaining = (download->content_length - download->data_received) / rate;
	memset(&tm, 0, sizeof(tm));
	tm.tm_sec = remaining;
	/* minutes */
	if(tm.tm_sec > 60)
	{
		tm.tm_min = tm.tm_sec / 60;
		tm.tm_sec = tm.tm_sec - (tm.tm_min * 60);
	}
	/* hours */
	if(tm.tm_min > 60)
	{
		tm.tm_hour = tm.tm_min / 60;
		tm.tm_min = tm.tm_min - (tm.tm_hour * 60);
	}
	strftime(buf, sizeof(buf), "%H:%M:%S", &tm);
	gtk_label_set_text(GTK_LABEL(download->remaining), buf);
}

static void _refresh_unit(guint64 size, double * fraction, char const ** unit,
		double * current)
{
	/* bytes */
	*fraction = size;
	*unit = _("bytes");
	if(*fraction < 1024)
		return;
	/* kilobytes */
	*fraction /= 1024;
	if(current != NULL)
		*current /= 1024;
	*unit = _("kB");
	if(*fraction < 1024)
		return;
	/* megabytes */
	*fraction /= 1024;
	if(current != NULL)
		*current /= 1024;
	*unit = _("MB");
}


/* download_write */
#if !defined(WITH_WEBKIT) && !defined(WITH_WEBKIT2)
static int _download_write(Download * download)
{
	gchar * buf;
	gsize size;
	gsize s;

	if(gnet_conn_http_steal_buffer(download->conn, &buf, &size) != TRUE)
		return 0;
	/* FIXME use a GIOChannel instead */
	s = fwrite(buf, sizeof(*buf), size, download->fp);
	g_free(buf);
	if(s == size)
	{
		download->pulse = 1;
		return 0;
	}
	_download_error(download, download->prefs.output, 0);
	download_cancel(download);
	return 1;
}
#endif


/* callbacks */
/* download_on_browse */
static void _download_on_browse(gpointer data)
{
	Download * download = data;
	GError * error = NULL;
	char * argv[] = { PROGNAME_BROWSER, NULL, NULL };

	if(download->prefs.output == NULL)
		return;
	argv[1] = dirname(download->prefs.output);
	if(g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL,
				NULL, &error) != TRUE)
		_download_error(download, error->message, 1);
}


/* download_on_cancel */
static void _download_on_cancel(gpointer data)
{
	Download * download = data;

	gtk_widget_hide(download->window);
	download_cancel(download);
}


/* download_on_closex */
static gboolean _download_on_closex(gpointer data)
{
	Download * download = data;

	gtk_widget_hide(download->window);
	download_cancel(download);
	return FALSE;
}


/* download_on_embedded */
static void _download_on_embedded(gpointer data)
{
	Download * download = data;

	gtk_widget_show(download->window);
}


/* download_on_http */
#if !defined(WITH_WEBKIT) && !defined(WITH_WEBKIT2)
static void _http_connected(Download * download);
static void _http_error(GConnHttpEventError * event, Download * download);
static void _http_data_complete(GConnHttpEventData * event,
		Download * download);
static void _http_data_partial(GConnHttpEventData * event, Download * download);
static void _http_redirect(GConnHttpEventRedirect * event, Download * download);
static void _http_resolved(GConnHttpEventResolved * event, Download * download);
static void _http_response(GConnHttpEventResponse * event, Download * download);
static void _http_timeout(Download * download);

static void _download_on_http(GConnHttp * conn, GConnHttpEvent * event,
		gpointer data)
{
	Download * download = data;

	if(download->conn != conn)
		return; /* FIXME report error */
	switch(event->type)
	{
		case GNET_CONN_HTTP_CONNECTED:
			_http_connected(download);
			break;
		case GNET_CONN_HTTP_ERROR:
			_http_error((GConnHttpEventError*)event, download);
			break;
		case GNET_CONN_HTTP_DATA_COMPLETE:
			_http_data_complete((GConnHttpEventData*)event,
					download);
			break;
		case GNET_CONN_HTTP_DATA_PARTIAL:
			_http_data_partial((GConnHttpEventData*)event,
					download);
			break;
		case GNET_CONN_HTTP_REDIRECT:
			_http_redirect((GConnHttpEventRedirect*)event,
					download);
			break;
		case GNET_CONN_HTTP_RESOLVED:
			_http_resolved((GConnHttpEventResolved*)event,
					download);
			break;
		case GNET_CONN_HTTP_RESPONSE:
			_http_response((GConnHttpEventResponse*)event,
					download);
			break;
		case GNET_CONN_HTTP_TIMEOUT:
			_http_timeout(download);
			break;
	}
}

static void _http_connected(Download * download)
{
	gtk_label_set_text(GTK_LABEL(download->status), _("Connected"));
	/* FIXME implement */
}

static void _http_error(GConnHttpEventError * event, Download * download)
{
	char buf[10];

	snprintf(buf, sizeof(buf), "%s %u", _("Error "), event->code);
	gtk_label_set_text(GTK_LABEL(download->status), buf);
	/* FIXME implement */
}

static void _http_data_complete(GConnHttpEventData * event,
		Download * download)
{
	g_source_remove(download->timeout);
	download->timeout = 0;
	if(_download_write(download) != 0)
		return;
	download->data_received = event->data_received;
	if(event->content_length == 0)
	download->content_length = (event->content_length != 0)
		? event->content_length : event->data_received;
	if(fclose(download->fp) != 0)
		_download_error(download, download->prefs.output, 0);
	download->fp = NULL;
	_download_refresh(download);
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(download->check)))
	{
		g_idle_add(_download_on_closex, download);
		return;
	}
	gtk_label_set_text(GTK_LABEL(download->status), _("Complete"));
	gtk_widget_set_sensitive(download->check, FALSE);
	gtk_button_set_label(GTK_BUTTON(download->cancel), GTK_STOCK_CLOSE);
	gtk_widget_show(download->browse);
}

static void _http_data_partial(GConnHttpEventData * event, Download * download)
{
	if(download->content_length == 0 && download->data_received == 0)
		gtk_label_set_text(GTK_LABEL(download->status),
				_("Downloading"));
	download->data_received = event->data_received;
	download->content_length = event->content_length;
	_download_write(download);
}

static void _http_redirect(GConnHttpEventRedirect * event, Download * download)
{
	char buf[256];

	if(event->new_location != NULL)
		snprintf(buf, sizeof(buf), "%s %s", _("Redirected to"),
				event->new_location);
	else
		strcpy(buf, _("Redirected"));
	gtk_label_set_text(GTK_LABEL(download->status), buf);
	/* FIXME implement */
}

static void _http_resolved(GConnHttpEventResolved * event, Download * download)
{
	gtk_label_set_text(GTK_LABEL(download->status), _("Resolved"));
	/* FIXME implement */
}

static void _http_response(GConnHttpEventResponse * event, Download * download)
{
	char buf[10];

	snprintf(buf, sizeof(buf), "%s %u", _("Code "), event->response_code);
	gtk_label_set_text(GTK_LABEL(download->status), buf);
	/* FIXME implement */
}

static void _http_timeout(Download * download)
{
	gtk_label_set_text(GTK_LABEL(download->status), _("Timeout"));
	/* FIXME implement */
}
#endif


/* download_on_idle */
static gboolean _download_on_idle(gpointer data)
{
	Download * download = data;
	DownloadPrefs * prefs = &download->prefs;
#if defined(WITH_WEBKIT)
	char * p = NULL;
	char * cwd = NULL;
	size_t len;
	WebKitNetworkRequest * request;

	download->timeout = 0;
	if(prefs->output[0] != '/' && (cwd = getcwd(NULL, 0)) == NULL)
	{
		_download_error(download, prefs->output, 0);
		download_cancel(download);
		return FALSE;
	}
	len = ((cwd != NULL) ? strlen(cwd) : 0) + strlen(prefs->output) + 7;
	if((p = malloc(len)) == NULL)
	{
		_download_error(download, prefs->output, 0);
		download_cancel(download);
		free(cwd);
		return FALSE;
	}
	snprintf(p, len, "%s%s%s%s", "file:", (cwd != NULL) ? cwd : "",
			(cwd != NULL) ? "/" : "", prefs->output);
	request = webkit_network_request_new(download->url);
	download->conn = webkit_download_new(request);
	webkit_download_set_destination_uri(download->conn, p);
	free(p);
	free(cwd);
	webkit_download_start(download->conn);
#elif defined(WITH_WEBKIT2)
	char * p = NULL;
	char * cwd = NULL;
	size_t len;
	WebKitNetworkRequest * request;

	download->timeout = 0;
	if(prefs->output[0] != '/' && (cwd = getcwd(NULL, 0)) == NULL)
	{
		_download_error(download, prefs->output, 0);
		download_cancel(download);
		return FALSE;
	}
	len = ((cwd != NULL) ? strlen(cwd) : 0) + strlen(prefs->output) + 7;
	if((p = malloc(len)) == NULL)
	{
		_download_error(download, prefs->output, 0);
		download_cancel(download);
		free(cwd);
		return FALSE;
	}
	snprintf(p, len, "%s%s%s%s", "file:", (cwd != NULL) ? cwd : "",
			(cwd != NULL) ? "/" : "", prefs->output);
	request = webkit_network_request_new(download->url);
	download->conn = webkit_download_new(request);
	webkit_download_set_destination(download->conn, p);
	free(p);
	free(cwd);
	webkit_download_start(download->conn);
#else
	download->timeout = 0;
	if((download->fp = fopen(prefs->output, "w")) == NULL)
	{
		_download_error(download, prefs->output, 0);
		download_cancel(download);
		return FALSE;
	}
	download->conn = gnet_conn_http_new();
	if(gnet_conn_http_set_method(download->conn, GNET_CONN_HTTP_METHOD_GET,
			NULL, 0) != TRUE)
		return _download_error(download, _("Unknown error"), FALSE);
	gnet_conn_http_set_uri(download->conn, download->url);
	if(prefs->user_agent != NULL)
		gnet_conn_http_set_user_agent(download->conn,
				prefs->user_agent);
	gnet_conn_http_run_async(download->conn, _download_on_http, download);
#endif
	download->timeout = g_timeout_add(250, _download_on_timeout, download);
	return FALSE;
}


/* download_on_timeout */
static gboolean _download_on_timeout(gpointer data)
{
	gboolean ret = TRUE;
	Download * d = data;
#if defined(WITH_WEBKIT) || defined(WITH_WEBKIT2)
	WebKitDownloadStatus status;
	guint64 received = d->data_received;

	/* FIXME not very efficient */
	status = webkit_download_get_status(d->conn);
	switch(status)
	{
		case WEBKIT_DOWNLOAD_STATUS_ERROR:
			ret = FALSE;
			gtk_label_set_text(GTK_LABEL(d->status), _("Error"));
			break;
		case WEBKIT_DOWNLOAD_STATUS_FINISHED:
			/* XXX pasted from _http_data_complete */
			ret = FALSE;
			gtk_label_set_text(GTK_LABEL(d->status), _("Complete"));
			gtk_widget_set_sensitive(d->check, FALSE);
			gtk_button_set_label(GTK_BUTTON(d->cancel),
					GTK_STOCK_CLOSE);
			gtk_widget_show(d->browse);
			d->data_received = webkit_download_get_current_size(
					d->conn);
			g_object_unref(d->conn);
			d->conn = NULL;
			if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
							d->check)))
			{
				g_idle_add(_download_on_closex, d);
				break;
			}
			break;
		case WEBKIT_DOWNLOAD_STATUS_STARTED:
			gtk_label_set_text(GTK_LABEL(d->status),
					_("Downloading"));
			d->data_received = webkit_download_get_current_size(
					d->conn);
			d->content_length = webkit_download_get_total_size(
					d->conn);
			if(d->content_length == d->data_received)
			{
				d->pulse = (d->data_received > received)
					? 1 : 0;
				d->content_length = 0;
			}
			break;
		default: /* XXX anything else to handle here? */
			break;
	}
#endif
	_download_refresh(d);
	if(ret != TRUE)
		d->timeout = 0;
	return ret;
}


#ifdef WITH_MAIN
/* error */
static int _error(char const * message, int ret)
{
	fputs(PROGNAME_DOWNLOAD ": ", stderr);
	perror(message);
	return ret;
}


/* usage */
static int _usage(void)
{
	fprintf(stderr, _("Usage: %s [-x][-O output][-U user-agent] URL...\n"
"  -x	Start in embedded mode\n"
"  -O	File to write the remote document to\n"
"  -U	User-agent string to send\n"), PROGNAME_DOWNLOAD);
	return 1;
}


/* main */
int main(int argc, char * argv[])
{
	DownloadPrefs prefs;
	int o;
	int cnt;
	char const * p;
	char http[256] = "";
	uint16_t port;
	Download * download;

	if(setlocale(LC_ALL, "") == NULL)
		_error("setlocale", 1);
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	memset(&prefs, 0, sizeof(prefs));
#if defined(WITH_GTKHTML) || defined(WITH_GTKTEXTVIEW) || defined(WITH_WEBKIT)
	if(g_thread_supported() == FALSE)
		g_thread_init(NULL);
#endif
	gtk_init(&argc, &argv);
	while((o = getopt(argc, argv, "O:U:x")) != -1)
		switch(o)
		{
			case 'O':
				prefs.output = optarg;
				break;
			case 'U':
				prefs.user_agent = optarg;
				break;
			case 'x':
				prefs.embedded = 1;
				break;
			default:
				return _usage();
		}
	if((cnt = argc - optind) == 0)
		return _usage();
	if((p = getenv("http_proxy")) != NULL
			&& sscanf(p, "http://%255[^:]:%hu", http, &port) == 2)
		http[sizeof(http) - 1] = '\0';
	else
		http[0] = '\0';
	for(o = 0; o < cnt; o++)
		if((download = download_new(&prefs, argv[optind + o])) != NULL)
			_download_set_proxy(download, http, port);
	gtk_main();
	return 0;
}
#endif /* WITH_MAIN */
