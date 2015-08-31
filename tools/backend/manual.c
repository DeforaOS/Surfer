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
int helper_open_manual(Helper * helper, char const * section, char const * page,
		char const * manhtmldir);


/* private */
/* constants */
static char const * _manual_prefix[] = { MANDIR, "/usr/share/man", NULL };


/* prototypes */
static void _new_manual(Helper * helper);

/* callbacks */
static void _helper_on_manual_row_activated(GtkWidget * widget,
		GtkTreePath * path, GtkTreeViewColumn * column, gpointer data);

/* filters */
static gboolean _helper_filter_manual(GtkTreeModel * model, GtkTreeIter * iter,
		gpointer data);


/* public */
/* functions */
/* helper_open_manual */
int helper_open_manual(Helper * helper, char const * section, char const * page,
		char const * manhtmldir)
{
	char const * prefix[] =
	{
		DATADIR "/man", PREFIX "/man", "/usr/local/share/man",
		"/usr/local/man", "/usr/share/man", "/usr/man", NULL
	};
	char const ** p;
	char buf[256];

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%d, \"%s\")\n", __func__, section, page);
#endif
	if(section == NULL)
		return -1;
	if(manhtmldir != NULL)
		p = &manhtmldir;
	else
		for(p = prefix; *p != NULL; p++)
		{
			snprintf(buf, sizeof(buf), "%s%s%s%s%s%s", *p,
					"/html", section, "/", page, ".html");
			if(access(buf, R_OK) == 0)
				break;
		}
	if(*p == NULL)
		return -1;
	/* read a manual page */
	snprintf(buf, sizeof(buf), "%s%s%s%s%s%s%s", "file://", *p, "/html",
			section, "/", page, ".html");
	return helper_open(helper, buf);
}


/* private */
/* functions */
/* new_manual */
static gboolean _new_manual_idle(gpointer data);
static void _new_manual_section(Helper * helper, char const * manhtmldir,
		char const * name, GtkTreeStore * store, char const * section);
static void _new_manual_section_lookup(GtkTreeStore * store, GtkTreeIter * iter,
		GdkPixbuf * pixbuf, char const * manhtmldir,
		char const * section, char const * name);

static void _new_manual(Helper * helper)
{
	GtkWidget * widget;
	GtkTreeModel * model;
	GtkCellRenderer * renderer;
	GtkTreeViewColumn * column;

	/* FIXME fully implement, de-duplicate code if possible */
	widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	model = gtk_tree_model_filter_new(GTK_TREE_MODEL(helper->store), NULL);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(model),
			_helper_filter_manual, NULL, NULL);
	model = gtk_tree_model_sort_new_with_model(model);
	helper->manual = gtk_tree_view_new_with_model(model);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(helper->manual), FALSE);
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(helper->manual),
			HSC_MANUAL_FILENAME);
	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new_with_attributes(NULL, renderer,
			"pixbuf", HSC_ICON, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(helper->manual), column);
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Section"),
			renderer, "text", HSC_MANUAL_FILENAME, NULL);
	gtk_tree_view_column_set_sort_column_id(column, HSC_MANUAL_FILENAME);
	gtk_tree_view_append_column(GTK_TREE_VIEW(helper->manual), column);
	gtk_tree_view_column_clicked(column);
	g_signal_connect(helper->manual, "row-activated", G_CALLBACK(
				_helper_on_manual_row_activated), helper);
	gtk_container_add(GTK_CONTAINER(widget), helper->manual);
	gtk_notebook_append_page(GTK_NOTEBOOK(helper->notebook), widget,
			gtk_label_new(_("Manual")));
}

static gboolean _new_manual_idle(gpointer data)
{
	Helper * helper = data;
	char const * p;
	DIR * dir;
	struct dirent * de;

	if(helper->p == NULL)
		helper->p = _manual_prefix;
	for(p = *(helper->p); p != NULL; (helper->p)++, p = *(helper->p))
	{
		/* XXX avoid duplicates */
		if((helper->p != &_manual_prefix[0]
					&& strcmp(p, _manual_prefix[0]) == 0)
				|| (helper->p != &_manual_prefix[1]
					&& strcmp(p, _manual_prefix[1]) == 0))
			continue;
		if((dir = opendir(p)) == NULL)
			continue;
		while((de = readdir(dir)) != NULL)
			if(strncasecmp(de->d_name, "html", 4) == 0
					&& de->d_name[4] != '\0')
				_new_manual_section(helper, p, de->d_name,
						helper->store, &de->d_name[4]);
		closedir(dir);
		(helper->p)++;
		return TRUE;
	}
	helper->source = 0;
	helper->p = NULL;
	return FALSE;
}

static void _new_manual_section(Helper * helper, char const * manhtmldir,
		char const * name, GtkTreeStore * store, char const * section)
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

	if((p = g_strdup_printf("%s/%s", manhtmldir, name)) == NULL)
		return;
	dir = opendir(p);
	g_free(p);
	if(dir == NULL)
		return;
	gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, &size, &size);
	pixbuf = gtk_icon_theme_load_icon(helper->icontheme, "folder", size, 0,
			NULL);
	_new_manual_section_lookup(store, &parent, pixbuf, manhtmldir, section,
			name);
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
				HSC_TYPE, HST_MANUAL, HSC_ICON, pixbuf,
				HSC_MANUAL_DIRECTORY, manhtmldir,
				HSC_MANUAL_SECTION, section,
				HSC_MANUAL_FILENAME, de->d_name, -1);
	}
	closedir(dir);
	if(pixbuf != NULL)
		g_object_unref(pixbuf);
}

static void _new_manual_section_lookup(GtkTreeStore * store, GtkTreeIter * iter,
		GdkPixbuf * pixbuf, char const * manhtmldir,
		char const * section, char const * name)
{
	GtkTreeModel * model = GTK_TREE_MODEL(store);
	gboolean valid;
	unsigned int type;
	gchar * n;
	int res;

	for(valid = gtk_tree_model_get_iter_first(model, iter); valid == TRUE;
			valid = gtk_tree_model_iter_next(model, iter))
	{
		gtk_tree_model_get(model, iter, HSC_TYPE, &type, -1);
		if(type != HST_MANUAL)
			continue;
		gtk_tree_model_get(model, iter, HSC_MANUAL_FILENAME, &n, -1);
		res = (n != NULL) && (strcmp(name, n) == 0);
		g_free(n);
		if(res != 0)
			break;
	}
	if(valid == FALSE)
	{
#if GTK_CHECK_VERSION(2, 10, 0)
		gtk_tree_store_insert_with_values(store, iter, NULL, -1,
#else
		gtk_tree_store_insert(store, iter, NULL, -1);
		gtk_tree_store_set(store, iter,
#endif
				HSC_TYPE, HST_MANUAL, HSC_ICON, pixbuf,
				HSC_MANUAL_DIRECTORY, manhtmldir,
				HSC_MANUAL_SECTION, section,
				HSC_MANUAL_FILENAME, name, -1);
	}
	else
		gtk_tree_store_set(store, iter, HSC_ICON, pixbuf,
				HSC_MANUAL_DIRECTORY, manhtmldir,
				HSC_MANUAL_SECTION, section,
				HSC_MANUAL_FILENAME, name, -1);
}


/* callbacks */
/* helper_on_manual_row_activated */
static void _helper_on_manual_row_activated(GtkWidget * widget,
		GtkTreePath * path, GtkTreeViewColumn * column, gpointer data)
{
	Helper * helper = data;
	GtkTreeModel * model;
	GtkTreeIter iter;
	GtkTreeIter parent;
	gchar * manhtmldir;
	gchar * section;
	gchar * command;

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
	gtk_tree_model_get(model, &iter, HSC_MANUAL_DIRECTORY, &manhtmldir,
			HSC_MANUAL_SECTION, &section,
			HSC_MANUAL_FILENAME, &command, -1);
	helper_open_manual(helper, section, command, manhtmldir);
	g_free(manhtmldir);
	g_free(section);
	g_free(command);
}


/* filters */
/* helper_filter_manual */
static gboolean _helper_filter_manual(GtkTreeModel * model, GtkTreeIter * iter,
		gpointer data)
{
	unsigned int type;

	gtk_tree_model_get(model, iter, HSC_TYPE, &type, -1);
	return (type == HST_MANUAL) ? TRUE : FALSE;
}
