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



/* private */
/* prototypes */
static void _new_search(Helper * helper);


/* callbacks */
static void _helper_on_search(gpointer data);
static void _helper_on_search_row_activated(GtkWidget * widget,
		GtkTreePath * path, GtkTreeViewColumn * column, gpointer data);


/* functions */
/* new_search */
static gboolean _new_search_filter(GtkTreeModel * model, GtkTreeIter * iter,
		gpointer data);
static gboolean _new_search_filter_do(GtkTreeModel * model, GtkTreeIter * iter,
		char const * search);
/* callbacks */
#if GTK_CHECK_VERSION(2, 16, 0)
static void _new_search_on_clear(gpointer data);
#endif

static void _new_search(Helper * helper)
{
	GtkWidget * vbox;
	GtkWidget * hbox;
	GtkWidget * widget;
	GtkTreeModel * model;
	GtkCellRenderer * renderer;
	GtkTreeViewColumn * column;

#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
	vbox = gtk_vbox_new(FALSE, 4);
#endif
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	helper->entry = gtk_entry_new();
#if GTK_CHECK_VERSION(2, 16, 0)
	gtk_entry_set_icon_from_stock(GTK_ENTRY(helper->entry),
			GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_CLEAR);
	g_signal_connect_swapped(helper->entry, "icon-press", G_CALLBACK(
				_new_search_on_clear), helper);
#endif
	g_signal_connect_swapped(helper->entry, "activate", G_CALLBACK(
				_helper_on_search), helper);
	gtk_box_pack_start(GTK_BOX(hbox), helper->entry, TRUE, TRUE, 0);
	widget = gtk_button_new_with_mnemonic(_("_Search"));
	g_signal_connect_swapped(widget, "clicked", G_CALLBACK(
				_helper_on_search), helper);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	model = gtk_tree_model_filter_new(GTK_TREE_MODEL(helper->store), NULL);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(model),
			_new_search_filter, helper, NULL);
	model = gtk_tree_model_sort_new_with_model(model);
	helper->search = gtk_tree_view_new_with_model(model);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(helper->search), FALSE);
	g_signal_connect(helper->search, "row-activated", G_CALLBACK(
				_helper_on_search_row_activated), helper);
	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new_with_attributes(NULL, renderer,
			"pixbuf", HSC_ICON, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(helper->search), column);
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(NULL, renderer,
			"text", HSC_DISPLAY, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(helper->search), column);
	gtk_container_add(GTK_CONTAINER(widget), helper->search);
	gtk_box_pack_start(GTK_BOX(vbox), widget, TRUE, TRUE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(helper->notebook), vbox,
			gtk_label_new(_("Search")));
}

static gboolean _new_search_filter(GtkTreeModel * model, GtkTreeIter * iter,
		gpointer data)
{
	Helper * helper = data;
	char const * search;
	GtkTreeIter child;
	gboolean valid;

	if((search = gtk_entry_get_text(GTK_ENTRY(helper->entry))) == NULL
			|| strlen(search) == 0)
		return FALSE;
	if(_new_search_filter_do(model, iter, search) == TRUE)
		return TRUE;
	for(valid = gtk_tree_model_iter_children(model, &child, iter);
			valid == TRUE;
			valid = gtk_tree_model_iter_next(model, &child))
		if(_new_search_filter_do(model, &child, search) == TRUE)
			return TRUE;
	return FALSE;
}

static gboolean _new_search_filter_do(GtkTreeModel * model, GtkTreeIter * iter,
		char const * search)
{
	gboolean ret;
	char * display;

	gtk_tree_model_get(model, iter, HSC_DISPLAY, &display, -1);
	if(display == NULL)
		return FALSE;
	ret = (strlen(display) > 0 && strcasestr(display, search) != NULL)
		? TRUE : FALSE;
	g_free(display);
	return ret;
}

#if GTK_CHECK_VERSION(2, 16, 0)
static void _new_search_on_clear(gpointer data)
{
	Helper * helper = data;

	gtk_entry_set_text(GTK_ENTRY(helper->entry), "");
}
#endif


/* callbacks */
/* helper_on_search */
static void _helper_on_search(gpointer data)
{
	Helper * helper = data;
	GtkTreeModel * model;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(helper->search));
	model = gtk_tree_model_sort_get_model(GTK_TREE_MODEL_SORT(model));
	gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(model));
}


/* helper_on_search_row_activated */
static void _helper_on_search_row_activated(GtkWidget * widget,
		GtkTreePath * path, GtkTreeViewColumn * column, gpointer data)
{
	Helper * helper = data;
	GtkTreeModel * model;
	GtkTreeIter iter;
	GtkTreeIter parent;
	unsigned int type;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_sort_convert_iter_to_child_iter(GTK_TREE_MODEL_SORT(
				model), &parent, &iter);
	model = gtk_tree_model_sort_get_model(GTK_TREE_MODEL_SORT(model));
	gtk_tree_model_filter_convert_iter_to_child_iter(GTK_TREE_MODEL_FILTER(
				model), &iter, &parent);
	model = GTK_TREE_MODEL(helper->store);
	if(gtk_tree_model_iter_parent(model, &parent, &iter) == FALSE)
	{
		if(gtk_tree_view_row_expanded(GTK_TREE_VIEW(widget), path))
			gtk_tree_view_collapse_row(GTK_TREE_VIEW(widget), path);
		else
			gtk_tree_view_expand_row(GTK_TREE_VIEW(widget), path,
					FALSE);
		return;
	}
	gtk_tree_model_get(model, &iter, HSC_TYPE, &type, -1);
	switch(type)
	{
		case HST_CONTENTS:
			_helper_on_contents_row_activated(widget, path, column,
					helper);
			break;
		case HST_GTKDOC:
			_helper_on_gtkdoc_row_activated(widget, path, column,
					helper);
			break;
		case HST_MANUAL:
			_helper_on_manual_row_activated(widget, path, column,
					helper);
			break;
	}
}
