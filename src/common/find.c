/* $Id$ */
/* Copyright (c) 2013-2014 Pierre Pronchery <khorben@defora.org> */
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
#if GTK_CHECK_VERSION(2, 16, 0)
static void _on_find_clear(gpointer data);
#endif
static void _on_find_clicked(gpointer data);
static void _on_find_hide(gpointer data);
static void _on_find_response(GtkWidget * widget, gint response, gpointer data);

void surfer_find(Surfer * surfer, char const * text)
{
	if(surfer->fi_dialog == NULL)
		_find_dialog(surfer);
	gtk_widget_grab_focus(surfer->fi_text);
	if(text != NULL)
		gtk_entry_set_text(GTK_ENTRY(surfer->fi_text), text);
	gtk_widget_show(surfer->fi_dialog);
}

static void _find_dialog(Surfer * surfer)
{
	GtkWidget * hbox;
	GtkWidget * widget;

#if GTK_CHECK_VERSION(3, 0, 0)
	surfer->fi_dialog = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	surfer->fi_dialog = gtk_hbox_new(FALSE, 4);
#endif
	hbox = surfer->fi_dialog;
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 4);
	/* text */
	widget = gtk_label_new(_("Text:"));
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	surfer->fi_text = gtk_entry_new();
	g_signal_connect_swapped(surfer->fi_text, "activate", G_CALLBACK(
				_on_find_clicked), surfer);
#if GTK_CHECK_VERSION(2, 16, 0)
	gtk_entry_set_icon_from_stock(GTK_ENTRY(surfer->fi_text),
			GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_CLEAR);
	g_signal_connect_swapped(surfer->fi_text, "icon-release", G_CALLBACK(
				_on_find_clear), surfer);
#endif
	gtk_box_pack_start(GTK_BOX(hbox), surfer->fi_text, FALSE, TRUE, 4);
	/* case-sensitive */
	surfer->fi_case = gtk_check_button_new_with_label(_("Case-sensitive"));
	gtk_box_pack_start(GTK_BOX(hbox), surfer->fi_case, FALSE, TRUE, 4);
	/* search backwards */
	surfer->fi_back = gtk_check_button_new_with_label(
			_("Search backwards"));
	gtk_box_pack_start(GTK_BOX(hbox), surfer->fi_back, FALSE, TRUE, 4);
	/* wrap */
	surfer->fi_wrap = gtk_check_button_new_with_label(_("Wrap"));
	gtk_box_pack_start(GTK_BOX(hbox), surfer->fi_wrap, FALSE, TRUE, 4);
	gtk_widget_show_all(hbox);
	g_signal_connect(surfer->fi_dialog, "response", G_CALLBACK(
				_on_find_response), surfer);
	/* find */
	widget = gtk_button_new_from_stock(GTK_STOCK_FIND);
	g_signal_connect_swapped(widget, "clicked", G_CALLBACK(
				_on_find_clicked), surfer);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	/* close */
	widget = gtk_button_new();
	gtk_button_set_image(GTK_BUTTON(widget), gtk_image_new_from_stock(
				GTK_STOCK_CLOSE, GTK_ICON_SIZE_BUTTON));
	gtk_button_set_relief(GTK_BUTTON(widget), GTK_RELIEF_NONE);
	g_signal_connect_swapped(widget, "clicked", G_CALLBACK(_on_find_hide),
			surfer);
	gtk_box_pack_end(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	gtk_widget_show_all(hbox);
	gtk_widget_hide(hbox);
	gtk_widget_set_no_show_all(hbox, TRUE);
	gtk_box_pack_end(GTK_BOX(surfer->vbox), hbox, FALSE, FALSE, 0);
}

#if GTK_CHECK_VERSION(2, 16, 0)
static void _on_find_clear(gpointer data)
{
	Surfer * surfer = data;

	gtk_entry_set_text(GTK_ENTRY(surfer->fi_text), "");
}
#endif

static void _on_find_clicked(gpointer data)
{
	Surfer * surfer = data;
	GtkWidget * view;
	char const * text;
	gboolean sensitive;
	gboolean backwards;
	gboolean wrap;

	if((view = surfer_get_view(surfer)) == NULL)
		return;
	if((text = gtk_entry_get_text(GTK_ENTRY(surfer->fi_text))) == NULL
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

static void _on_find_hide(gpointer data)
{
	Surfer * surfer = data;

	gtk_widget_hide(surfer->fi_dialog);
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
	_on_find_clicked(surfer);
}
