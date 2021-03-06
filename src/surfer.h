/* $Id$ */
/* Copyright (c) 2006-2016 Pierre Pronchery <khorben@defora.org> */
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



#ifndef SURFER_SURFER_H
# define SURFER_SURFER_H

# include <stdint.h>
# include <System.h>
# include <gtk/gtk.h>


/* Surfer */
/* constants */
# define SURFER_CONFIG_FILE			".surferrc"

# define SURFER_DEFAULT_MINIMUM_FONT_SIZE	8.0
# define SURFER_DEFAULT_FONT_SIZE		12.0
# define SURFER_DEFAULT_FIXED_FONT_SIZE		12.0
# define SURFER_DEFAULT_ENCODING		"ISO-8859-1"
# define SURFER_DEFAULT_SERIF_FONT		"Serif"
# define SURFER_DEFAULT_SANS_FONT		"Sans"
# define SURFER_DEFAULT_STANDARD_FONT		SURFER_DEFAULT_SANS_FONT
# define SURFER_DEFAULT_FIXED_FONT		"Monospace"
# define SURFER_DEFAULT_FANTASY_FONT		"Comic Sans MS"
# define SURFER_DEFAULT_TITLE			"Web surfer"


/* types */
typedef struct _Surfer Surfer;

typedef enum _SurferSecurity
{
	SS_NONE = 0,
	SS_TRUSTED,
	SS_UNTRUSTED
}
SurferSecurity;

typedef enum _SurferProxyType
{
	SPT_NONE = 0,
	SPT_HTTP
} SurferProxyType;


/* functions */
Surfer * surfer_new(char const * url);
Surfer * surfer_new_copy(Surfer * surfer);
void surfer_delete(Surfer * surfer);


/* accessors */
GtkWidget * surfer_get_view(Surfer * surfer);

void surfer_set_enable_javascript(Surfer * surfer, gboolean enable);
void surfer_set_favicon(Surfer * surfer, GdkPixbuf * pixbuf);
void surfer_set_fullscreen(Surfer * surfer, gboolean fullscreen);
void surfer_set_homepage(Surfer * surfer, char const * homepage);
void surfer_set_location(Surfer * surfer, char const * url);
void surfer_set_progress(Surfer * surfer, gdouble fraction);
void surfer_set_proxy(Surfer * surfer, SurferProxyType type, char const * http,
		uint16_t http_port);
void surfer_set_security(Surfer * surfer, SurferSecurity security);
void surfer_set_status(Surfer * surfer, char const * status);
void surfer_set_title(Surfer * surfer, char const * title);
void surfer_set_user_agent(Surfer * surfer, char const * user_agent);
void surfer_set_zoom(Surfer * surfer, gdouble zoom);


/* useful */
int surfer_config_load(Surfer * surfer);
int surfer_config_save(Surfer * surfer);

int surfer_confirm(Surfer * surfer, char const * message, gboolean * confirmed);
void surfer_console_clear(Surfer * surfer);
void surfer_console_execute(Surfer * surfer);
void surfer_console_message(Surfer * surfer, char const * message,
		char const * source, long line);
int surfer_error(Surfer * surfer, char const * message, int ret);
int surfer_prompt(Surfer * surfer, char const * message,
		char const * default_value, char ** value);
void surfer_warning(Surfer * surfer, char const * message);

void surfer_open(Surfer * surfer, char const * url);
void surfer_open_dialog(Surfer * surfer);
void surfer_open_tab(Surfer * surfer, char const * url);
void surfer_close_tab(Surfer * surfer, GtkWidget * widget);

void surfer_print(Surfer * surfer);

void surfer_save(Surfer * surfer, char const * filename);
void surfer_save_dialog(Surfer * surfer);

void surfer_find(Surfer * surfer, char const * text);

/* download */
int surfer_download(Surfer * surfer, char const * url, char const * suggested);

/* edition */
void surfer_copy(Surfer * surfer);
void surfer_cut(Surfer * surfer);
void surfer_paste(Surfer * surfer);
void surfer_redo(Surfer * surfer);
void surfer_undo(Surfer * surfer);

/* interface */
void surfer_about(Surfer * surfer);
void surfer_resize(Surfer * surfer, gint width, gint height);
void surfer_show_console(Surfer * surfer, gboolean show);
void surfer_show_location(Surfer * surfer, gboolean show);
void surfer_show_menubar(Surfer * surfer, gboolean show);
void surfer_show_statusbar(Surfer * surfer, gboolean show);
void surfer_show_toolbar(Surfer * surfer, gboolean show);
void surfer_show_window(Surfer * surfer, gboolean show);

/* location */
gboolean surfer_go_back(Surfer * surfer);
gboolean surfer_go_forward(Surfer * surfer);
void surfer_go_home(Surfer * surfer);

/* loading */
void surfer_refresh(Surfer * surfer);
void surfer_reload(Surfer * surfer);
void surfer_stop(Surfer * surfer);

void surfer_view_preferences(Surfer * surfer);
void surfer_view_security(Surfer * surfer);
void surfer_view_source(Surfer * surfer);

/* selection */
void surfer_select_all(Surfer * surfer);
void surfer_unselect_all(Surfer * surfer);

/* zoom */
void surfer_zoom_in(Surfer * surfer);
void surfer_zoom_out(Surfer * surfer);
void surfer_zoom_reset(Surfer * surfer);

#endif /* !SURFER_SURFER_H */
