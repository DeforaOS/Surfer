/* $Id$ */
/* Copyright (c) 2013 Pierre Pronchery <khorben@defora.org> */
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



/* surfer_find */
static void _find_dialog(Surfer * surfer);
static void _on_find_activate(GtkWidget * widget, gpointer data);
static void _on_find_response(GtkWidget * widget, gint response, gpointer data);

void surfer_find(Surfer * surfer, char const * text)
{
	if(surfer->fi_dialog == NULL)
		_find_dialog(surfer);
	gtk_widget_grab_focus(surfer->fi_text);
	if(text != NULL)
		gtk_entry_set_text(GTK_ENTRY(surfer->fi_text), text);
	gtk_window_present(GTK_WINDOW(surfer->fi_dialog));
}

static void _find_dialog(Surfer * surfer)
{
	GtkWidget * vbox;
	GtkWidget * hbox;
	GtkWidget * widget;

	surfer->fi_dialog = gtk_dialog_new_with_buttons(_("Find text"),
			GTK_WINDOW(surfer->window),
			GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
			GTK_STOCK_FIND, GTK_RESPONSE_ACCEPT, NULL);
#if GTK_CHECK_VERSION(2, 14, 0)
	vbox = gtk_dialog_get_content_area(GTK_DIALOG(surfer->fi_dialog));
#else
	vbox = GTK_DIALOG(surfer->fi_dialog)->vbox;
#endif
	/* text */
	hbox = gtk_hbox_new(FALSE, 0);
	widget = gtk_label_new(_("Text:"));
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	surfer->fi_text = gtk_entry_new();
	g_signal_connect(G_OBJECT(surfer->fi_text), "activate", G_CALLBACK(
				_on_find_activate), surfer);
	gtk_box_pack_start(GTK_BOX(hbox), surfer->fi_text, TRUE, TRUE, 4);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 4);
	/* case-sensitive */
	surfer->fi_case = gtk_check_button_new_with_label(_("Case-sensitive"));
	gtk_box_pack_start(GTK_BOX(vbox), surfer->fi_case, TRUE, TRUE, 4);
	/* search backwards */
	surfer->fi_back = gtk_check_button_new_with_label(
			_("Search backwards"));
	gtk_box_pack_start(GTK_BOX(vbox), surfer->fi_back, TRUE, TRUE, 4);
	/* wrap */
	surfer->fi_wrap = gtk_check_button_new_with_label(_("Wrap"));
	gtk_box_pack_start(GTK_BOX(vbox), surfer->fi_wrap, TRUE, TRUE, 4);
	gtk_widget_show_all(vbox);
	g_signal_connect(G_OBJECT(surfer->fi_dialog), "response", G_CALLBACK(
				_on_find_response), surfer);
}

static void _on_find_activate(GtkWidget * widget, gpointer data)
{
	Surfer * surfer = data;
	GtkWidget * view;
	char const * text;
	gboolean sensitive;
	gboolean backwards;
	gboolean wrap;

	if((view = surfer_get_view(surfer)) == NULL)
		return;
	if((text = gtk_entry_get_text(GTK_ENTRY(widget))) == NULL
			|| strlen(text) == 0)
		return;
	sensitive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
				surfer->fi_case));
	backwards = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
				surfer->fi_back));
	wrap = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
				surfer->fi_wrap));
	if(ghtml_find(view, text, sensitive, backwards, wrap) == TRUE)
		return;
	/* FIXME display this error on top of the text search box instead */
	surfer_error(surfer, _("Text not found"), 0);
}

static void _on_find_response(GtkWidget * widget, gint response, gpointer data)
{
	Surfer * surfer = data;

	if(response != GTK_RESPONSE_ACCEPT)
	{
		gtk_widget_hide(widget);
		if(response == GTK_RESPONSE_DELETE_EVENT)
			surfer->fi_dialog = NULL;
		return;
	}
	_on_find_activate(surfer->fi_text, surfer);
}
