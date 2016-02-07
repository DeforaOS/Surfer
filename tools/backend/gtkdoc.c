/* $Id$ */
/* Copyright (c) 2015-2016 Pierre Pronchery <khorben@defora.org> */
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
int helper_open_gtkdoc(Helper * helper, char const * gtkdocdir,
		char const * package);


/* private */
/* constants */
static char const * _gtkdoc_prefix[] =
{
	/* FIXME look into more directories */
	DATADIR "/gtk-doc/html", DATADIR "/devhelp/books",
	"/usr/local/share/gtk-doc/html",
	"/usr/local/share/devhelp/books",
	"/usr/share/gtk-doc/html", "/usr/share/devhelp/books", NULL
};


/* prototypes */
static void _new_gtkdoc(Helper * helper);

/* callbacks */
static void _helper_on_gtkdoc_row_activated(GtkWidget * widget,
		GtkTreePath * path, GtkTreeViewColumn * column, gpointer data);

/* filters */
static gboolean _helper_filter_gtkdoc(GtkTreeModel * model, GtkTreeIter * iter,
		gpointer data);


/* public */
/* functions */
/* helper_open_gtkdoc */
int helper_open_gtkdoc(Helper * helper, char const * gtkdocdir,
		char const * package)
{
	int ret;
	char const * prefix[] =
	{
		DATADIR "/gtk-doc/html", DATADIR "/devhelp/books",
		"/usr/local/share/gtk-doc/html",
		"/usr/local/share/devhelp/books",
		"/usr/share/gtk-doc/html", "/usr/share/devhelp/books", NULL
	};
	char const ** p;
	String * s;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\", \"%s\")\n", __func__, gtkdocdir,
			package);
#endif
	for(p = prefix; gtkdocdir == NULL && *p != NULL; p++)
	{
		if((s = string_new_append(*p, "/", package, "/index.html",
						NULL)) == NULL)
			return -1;
		ret = access(s, R_OK);
		string_delete(s);
		if(ret == 0)
			break;
	}
	if(gtkdocdir != NULL)
		p = &gtkdocdir;
	if(*p == NULL)
		return -1;
	/* read the API documentation */
	if((s = string_new_append("file://", *p, "/", package, "/index.html",
					NULL)) == NULL)
		return -1;
	ret = helper_open(helper, s);
	string_delete(s);
	return ret;
}


/* private */
/* functions */
/* new_gtkdoc */
static gboolean _new_gtkdoc_idle(gpointer data);
static void _new_gtkdoc_package(Helper * helper, char const * gtkdocdir,
		GtkTreeStore * store, char const * package);

static void _new_gtkdoc(Helper * helper)
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
			_helper_filter_gtkdoc, NULL, NULL);
	model = gtk_tree_model_sort_new_with_model(model);
	helper->gtkdoc = gtk_tree_view_new_with_model(model);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(helper->gtkdoc), FALSE);
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(helper->gtkdoc),
			HSC_GTKDOC_PACKAGE);
	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new_with_attributes(NULL, renderer,
			"pixbuf", HSC_ICON, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(helper->gtkdoc), column);
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Package"),
			renderer, "text", HSC_GTKDOC_PACKAGE, NULL);
	gtk_tree_view_column_set_sort_column_id(column, HSC_GTKDOC_PACKAGE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(helper->gtkdoc), column);
	gtk_tree_view_column_clicked(column);
	g_signal_connect(helper->gtkdoc, "row-activated", G_CALLBACK(
				_helper_on_gtkdoc_row_activated), helper);
	gtk_container_add(GTK_CONTAINER(widget), helper->gtkdoc);
	gtk_notebook_append_page(GTK_NOTEBOOK(helper->notebook), widget,
			gtk_label_new(_("API reference")));
	helper->source = g_idle_add(_new_gtkdoc_idle, helper);
}

static gboolean _new_gtkdoc_idle(gpointer data)
{
	Helper * helper = data;
	char const * p;
	DIR * dir;
	struct dirent * de;

	if(helper->p == NULL)
		helper->p = _gtkdoc_prefix;
	for(p = *(helper->p); p != NULL; (helper->p)++, p = *(helper->p))
	{
		/* XXX avoid duplicates */
		if((helper->p != &_gtkdoc_prefix[0]
					&& strcmp(p, _gtkdoc_prefix[0]) == 0)
				|| (helper->p != &_gtkdoc_prefix[1]
					&& strcmp(p, _gtkdoc_prefix[1]) == 0))
			continue;
		if((dir = opendir(p)) == NULL)
			continue;
		while((de = readdir(dir)) != NULL)
			if(de->d_name[0] != '.')
				_new_gtkdoc_package(helper, p, helper->store,
						de->d_name);
		closedir(dir);
		(helper->p)++;
		return TRUE;
	}
	helper->source = g_idle_add(_new_contents_idle, helper);
	helper->p = NULL;
	return FALSE;
}

static void _new_gtkdoc_package(Helper * helper, char const * gtkdocdir,
		GtkTreeStore * store, char const * package)
{
	gchar * p;
	FILE * fp;
	GtkTreeIter parent;
	GtkTreeIter iter;
	gint size = 16;
	GdkPixbuf * pixbuf;

	if((p = g_strdup_printf("%s/%s/%s.devhelp2", gtkdocdir, package,
					package)) == NULL)
		return;
	if((fp = fopen(p, "r")) == NULL)
	{
		_error(p, 1);
		g_free(p);
		if((p = g_strdup_printf("%s/%s/%s.devhelp", gtkdocdir, package,
						package)) == NULL)
			return;
	}
	if(fp == NULL && (fp = fopen(p, "r")) == NULL)
		_error(p, 1);
	g_free(p);
	if(fp == NULL)
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
			HSC_TYPE, HST_GTKDOC, HSC_ICON, pixbuf,
			HSC_GTKDOC_PACKAGE, package, -1);
	if(pixbuf != NULL)
	{
		g_object_unref(pixbuf);
		pixbuf = NULL;
	}
	if(pixbuf == NULL)
		pixbuf = gtk_icon_theme_load_icon(helper->icontheme,
				"help-contents", size, 0, NULL);
	/* FIXME parse the contents of the devhelp(2) file */
#if GTK_CHECK_VERSION(2, 10, 0)
	gtk_tree_store_insert_with_values(store, &iter, &parent, -1,
#else
	gtk_tree_store_insert(store, &iter, &parent, -1);
	gtk_tree_store_set(store, &iter,
#endif
			HSC_TYPE, HST_GTKDOC, HSC_ICON, pixbuf,
			HSC_GTKDOC_PACKAGE, package,
			HSC_GTKDOC_DIRECTORY, gtkdocdir, -1);
	if(pixbuf != NULL)
		g_object_unref(pixbuf);
	fclose(fp);
}


/* callbacks */
/* helper_on_gtkdoc_row_activated */
static void _helper_on_gtkdoc_row_activated(GtkWidget * widget,
		GtkTreePath * path, GtkTreeViewColumn * column, gpointer data)
{
	Helper * helper = data;
	GtkTreeModel * model;
	GtkTreeIter iter;
	GtkTreeIter parent;
	gchar * package;
	gchar * gtkdocdir;
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
	gtk_tree_model_get(model, &iter, HSC_GTKDOC_PACKAGE, &package,
			HSC_GTKDOC_DIRECTORY, &gtkdocdir, -1);
	helper_open_gtkdoc(helper, gtkdocdir, package);
	g_free(package);
	g_free(gtkdocdir);
}


/* helper_filter_gtkdoc */
static gboolean _helper_filter_gtkdoc(GtkTreeModel * model, GtkTreeIter * iter,
		gpointer data)
{
	unsigned int type;
	(void) data;

	gtk_tree_model_get(model, iter, HSC_TYPE, &type, -1);
	return (type == HST_GTKDOC) ? TRUE : FALSE;
}
