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




/* public */
/* prototypes */
int helper_open_contents(Helper * helper, char const * package,
		char const * command);


/* private */
/* prototypes */
static void _new_contents(Helper * helper);

/* callbacks */
static void _helper_on_contents_row_activated(GtkWidget * widget,
		GtkTreePath * path, GtkTreeViewColumn * column, gpointer data);

/* filters */
static gboolean _helper_filter_contents(GtkTreeModel * model,
		GtkTreeIter * iter, gpointer data);


/* public */
/* functions */
/* helper_open_contents */
int helper_open_contents(Helper * helper, char const * package,
		char const * command)
{
	char buf[256];

	if(package == NULL)
		return -1;
	if(command == NULL)
		command = "index";
	/* read a package documentation */
	snprintf(buf, sizeof(buf), "%s%s%s%s%s", "file://" CONTENTSDIR "/",
			package, "/", command, ".html");
	return helper_open(helper, buf);
}


/* functions */
/* new_contents */
static gboolean _new_contents_idle(gpointer data);
static void _new_contents_package(Helper * helper, char const * contentsdir,
		GtkTreeStore * store, char const * package);

static void _new_contents(Helper * helper)
{
	GtkWidget * widget;
	GtkTreeModel * model;
	GtkCellRenderer * renderer;
	GtkTreeViewColumn * column;

	widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	model = gtk_tree_model_filter_new(GTK_TREE_MODEL(helper->store), NULL);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(model),
			_helper_filter_contents, NULL, NULL);
	model = gtk_tree_model_sort_new_with_model(model);
	helper->contents = gtk_tree_view_new_with_model(model);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(helper->contents),
			FALSE);
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(helper->contents),
			HSC_CONTENTS_PACKAGE);
	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new_with_attributes(NULL, renderer,
			"pixbuf", HSC_ICON, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(helper->contents), column);
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Package"),
			renderer, "text", HSC_CONTENTS_PACKAGE, NULL);
	gtk_tree_view_column_set_sort_column_id(column, HSC_CONTENTS_PACKAGE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(helper->contents), column);
	gtk_tree_view_column_clicked(column);
	g_signal_connect(helper->contents, "row-activated", G_CALLBACK(
				_helper_on_contents_row_activated), helper);
	gtk_container_add(GTK_CONTAINER(widget), helper->contents);
	gtk_notebook_append_page(GTK_NOTEBOOK(helper->notebook), widget,
			gtk_label_new(_("Contents")));
}

static gboolean _new_contents_idle(gpointer data)
{
	Helper * helper = data;
	DIR * dir;
	struct dirent * de;

	helper->source = g_idle_add(_new_manual_idle, helper);
	helper->p = NULL;
	if((dir = opendir(CONTENTSDIR)) == NULL)
		return FALSE;
	while((de = readdir(dir)) != NULL)
		if(de->d_name[0] != '.')
			_new_contents_package(helper, CONTENTSDIR,
					helper->store, de->d_name);
	closedir(dir);
	return FALSE;
}

static void _new_contents_package(Helper * helper, char const * contentsdir,
		GtkTreeStore * store, char const * package)
{
	const char ext[] = ".html";
	gchar * p;
	DIR * dir;
	struct dirent * de;
	size_t len;
	GtkTreeIter parent;
	GtkTreeIter iter;
	gint size = 16;
	GdkPixbuf * pixbuf;

	if((p = g_strdup_printf("%s/%s", contentsdir, package)) == NULL)
		return;
	dir = opendir(p);
	g_free(p);
	if(dir == NULL)
		return;
	gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, &size, &size);
	pixbuf = gtk_icon_theme_load_icon(helper->icontheme, "folder", size, 0,
			NULL);
#if GTK_CHECK_VERSION(2, 10, 0)
	gtk_tree_store_insert_with_values(store, &parent, NULL, -1,
#else
	gtk_tree_store_insert(store, &parent, NULL, -1);
	gtk_tree_store_set(store, &parent,
#endif
			HSC_TYPE, HST_CONTENTS, HSC_ICON, pixbuf,
			HSC_CONTENTS_PACKAGE, package, -1);
	if(pixbuf != NULL)
	{
		g_object_unref(pixbuf);
		pixbuf = NULL;
	}
	while((de = readdir(dir)) != NULL)
	{
		if(de->d_name[0] == '.'
				|| (len = strlen(de->d_name)) < sizeof(ext)
				|| strcmp(&de->d_name[len - sizeof(ext) + 1],
					ext) != 0)
			continue;
		de->d_name[len - sizeof(ext) + 1] = '\0';
		if(pixbuf == NULL)
			pixbuf = gtk_icon_theme_load_icon(helper->icontheme,
					"help-contents", size, 0, NULL);
#if GTK_CHECK_VERSION(2, 10, 0)
		gtk_tree_store_insert_with_values(store, &iter, &parent, -1,
#else
		gtk_tree_store_insert(store, &iter, &parent, -1);
		gtk_tree_store_set(store, &iter,
#endif
				HSC_TYPE, HST_CONTENTS, HSC_ICON, pixbuf,
				HSC_CONTENTS_PACKAGE, de->d_name, -1);
	}
	closedir(dir);
	if(pixbuf != NULL)
		g_object_unref(pixbuf);
}


/* callbacks */
/* helper_on_contents_row_activated */
static void _helper_on_contents_row_activated(GtkWidget * widget,
		GtkTreePath * path, GtkTreeViewColumn * column, gpointer data)
{
	Helper * helper = data;
	GtkTreeModel * model;
	GtkTreeIter iter;
	GtkTreeIter parent;
	gchar * package;
	gchar * command;
	(void) column;

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
	gtk_tree_model_get(model, &parent, HSC_CONTENTS_PACKAGE, &package, -1);
	gtk_tree_model_get(model, &iter, HSC_CONTENTS_PACKAGE, &command, -1);
	helper_open_contents(helper, package, command);
	g_free(package);
	g_free(command);
}


/* filters */
/* helper_filter_contents */
static gboolean _helper_filter_contents(GtkTreeModel * model,
		GtkTreeIter * iter, gpointer data)
{
	unsigned int type;
	(void) data;

	gtk_tree_model_get(model, iter, HSC_TYPE, &type, -1);
	return (type == HST_CONTENTS) ? TRUE : FALSE;
}
