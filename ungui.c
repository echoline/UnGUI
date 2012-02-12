/* UnGUI shell application
 * Copyright (C) 2012 Eli Cohen
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <gtk/gtk.h>
#include <stdlib.h>

/* type representation of program and data items */

typedef struct _Object Object;
struct _Object {
	GtkWidget *window;
	Object *next;
	Object *prev;
	GtkWidget *data;
	char *cmd;
	int stdin;
	int stdout;
};
Object *selected = NULL;
GtkWidget *selbutton = NULL;
GList *objects = NULL;

static GtkWidget* new_data_window(GtkTextBuffer **);

/* g_signal_connect callbacks */

static void connectobj(GtkWidget *button, gpointer window) {
	GList *iter = objects;
	Object *data;

	while (iter != NULL) {
		data = iter->data;
		if (window == data->window)
			break;
		iter = iter->next;
	}

	if (iter == NULL || iter->data == selected) {
		gtk_drag_unhighlight(button);
		if (selected != NULL)
			selected->prev = selected->next = NULL;
		selected = NULL;
		selbutton = NULL;
	} else if (selbutton != NULL) {
		gtk_drag_unhighlight(selbutton);
		selected->next = data;
		data->prev = selected;
		selected = NULL;
		selbutton = NULL;
	} else {
		gtk_drag_highlight(button);
		selected = iter->data;
		selbutton = button;
	}
}

static void syspath(GtkWidget *button, gpointer window) {
	GtkWidget *dialog = gtk_dialog_new_with_buttons("Select Utility", NULL, 0, NULL);
	GtkWidget *vbox = gtk_vbox_new(FALSE, 2);
	GtkWidget *text = gtk_entry_new();

	gtk_box_pack_start ((GtkBox*)vbox, text, TRUE, TRUE, 1);
	gtk_container_add((GtkContainer*)gtk_dialog_get_content_area((GtkDialog*)dialog), vbox);

	gtk_dialog_add_button((GtkDialog*)dialog, "gtk-cancel", GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button((GtkDialog*)dialog, "gtk-open", GTK_RESPONSE_ACCEPT);

	gtk_widget_show_all(dialog);
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		GtkWidget *program = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		const gchar *cmd = gtk_entry_get_text((GtkEntry*)text);
		Object *data = calloc(1, sizeof(Object));
		data->cmd = g_strdup(cmd);
		gtk_window_set_title(GTK_WINDOW(program), cmd);
		gtk_widget_set_size_request(program, 320, -1);

		GtkWidget *hbox = gtk_hbox_new (FALSE, 1);
		GtkWidget *toolbar = gtk_toolbar_new();

		data->window = program;
		objects = g_list_append(objects, data);

		GtkWidget *button = (GtkWidget*)gtk_tool_button_new_from_stock("gtk-connect");
		g_signal_connect(button, "clicked", (GCallback)connectobj, program);
		gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), -1);

		gtk_box_pack_start ((GtkBox*)hbox, toolbar, TRUE, TRUE, 0);
		gtk_container_add ((GtkContainer*)program, hbox);
		gtk_widget_show_all(program);
	}

	gtk_widget_destroy (dialog);
}

static void opendata(GtkWidget *button, gpointer __unused) {
	GtkWidget *window;
	GtkTextBuffer *buffer;
	GtkTextIter start;
	gchar *contents;
	GtkWidget *dialog = gtk_file_chooser_dialog_new ("Open Data from File",
				NULL,
				GTK_FILE_CHOOSER_ACTION_OPEN,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				NULL);
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename;
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

		if (g_file_get_contents(filename, &contents, NULL, NULL)) {
			window = new_data_window(&buffer);
			gtk_window_set_title(GTK_WINDOW(window), filename);
			gtk_text_buffer_get_start_iter(buffer, &start);
			gtk_text_buffer_insert(buffer, &start, contents, -1);
		}

		g_free (filename);
	}
	gtk_widget_destroy (dialog);
}

static void newdata(GtkWidget *button, gpointer __unused) {
	GtkTextBuffer *buffer;
	GtkWidget *window = new_data_window(&buffer);
	gtk_window_set_title(GTK_WINDOW(window), "[new data]");
}

static void savedata(GtkWidget *button, gpointer window) {
	GtkWidget *dialog;
	GtkTextBuffer *buffer;
	char *filename;
	gchar *contents;
	GtkTextIter start, end;
	GList *iter = objects;
	Object *data;

	while (iter != NULL) {
		data = iter->data;
		if (window == data->window)
			break;
		iter = iter->next;
	}

	if (iter == NULL)
		return;

	dialog = gtk_file_chooser_dialog_new ("Save Data to File",
				NULL,
				GTK_FILE_CHOOSER_ACTION_SAVE,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
				NULL);
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		buffer = gtk_text_view_get_buffer((GtkTextView*)data->data);
		gtk_text_buffer_get_start_iter(buffer, &start);
		gtk_text_buffer_get_end_iter(buffer, &end);
		contents = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
		g_file_set_contents(filename, contents, -1, NULL);
		gtk_window_set_title(GTK_WINDOW(data->window), filename);
		g_free (filename);
	}
	gtk_widget_destroy (dialog);
}

static void closedata(GtkWidget *__unused, gpointer textview) {
	GList *iter = objects;
	Object *data;

	while (iter != NULL) {
		data = iter->data;
		if (textview == data->data)
			break;
		iter = iter->next;
	}

	if (iter != NULL) {
		objects = g_list_remove(objects, data);
		free(data);
	}
}

static void execute(GtkWidget *button, gpointer __unused) {
	GList *iter = objects;
	GList *starts = NULL;
	Object *data, *last = NULL;
	GtkTextBuffer *buffer;
	GtkTextIter start, end;
	char *contents = NULL;
	gint len;

	while (iter != NULL) {
		data = iter->data;
		if (data->prev == NULL)
			starts = g_list_append(starts, data);
		iter = iter->next;
	}

	while (starts != NULL) {
		data = starts->data;
		starts = g_list_remove(starts, data);
		while (data != NULL) {
			if (data->data == NULL) {
				gint argc;
				char **argv;
				if (!g_shell_parse_argv(data->cmd, &argc, &argv, NULL))
					return;

				if (!g_spawn_async_with_pipes(NULL, argv, NULL, 0, NULL, NULL, NULL,
								&data->stdin, &data->stdout, NULL, NULL))
					return;

				if (last == NULL) {
					close(data->stdin);
				} else if (last->data == NULL) {
					char c;
					while (read(last->stdout, &c, 1) > 0)
						write(data->stdin, &c, 1);
					close(data->stdin);
				} else {
					buffer = gtk_text_view_get_buffer((GtkTextView*)last->data);
					len = gtk_text_buffer_get_char_count(buffer);
					gtk_text_buffer_get_start_iter(buffer, &start);
					gtk_text_buffer_get_end_iter(buffer, &end);
					contents = gtk_text_buffer_get_text(buffer, &start, &end, 0);
					write(data->stdin, contents, len);
					close(data->stdin);
				}
			} else {
				if (last != NULL) {
					if (last->data == NULL) {
						len = 0;
						char c;
						while (read(last->stdout, &c, 1) > 0) {
							len++;
							contents = realloc(contents, len+1);
							contents[len-1] = c;
						}
						contents[len] = 0;
						buffer = gtk_text_view_get_buffer((GtkTextView*)data->data);
						gtk_text_buffer_set_text(buffer, contents, -1);
					} else {
						buffer = gtk_text_view_get_buffer((GtkTextView*)last->data);
						len = gtk_text_buffer_get_char_count(buffer);
						gtk_text_buffer_get_start_iter(buffer, &start);
						gtk_text_buffer_get_end_iter(buffer, &end);
						contents = gtk_text_buffer_get_text(buffer, &start, &end, 0);
						buffer = gtk_text_view_get_buffer((GtkTextView*)data->data);
						gtk_text_buffer_set_text(buffer, contents, -1);
					}
				}
			}
			last = data;
			data = data->next;
		}
	}
}

/* end of g_signal_connect callbacks */


static GtkWidget* new_data_window(GtkTextBuffer **buffer) {
	GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	GtkWidget *vbox = gtk_vbox_new (FALSE, 2);
	GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
	GtkWidget *textview = gtk_text_view_new();
	GtkWidget *toolbar;
	GtkWidget *button;

	Object *data = calloc(1, sizeof(Object));
	data->data = textview;
	data->window = window;

	g_signal_connect(window, "destroy", (GCallback)closedata, textview);
	gtk_widget_set_size_request(window, 480, 320);
	gtk_container_border_width (GTK_CONTAINER (vbox), 1);
	gtk_container_add (GTK_CONTAINER (window), vbox);

	toolbar = gtk_toolbar_new();

	button = (GtkWidget*)gtk_tool_button_new_from_stock("gtk-connect");
	g_signal_connect(button, "clicked", (GCallback)connectobj, window);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), -1);

	button = (GtkWidget*)gtk_tool_button_new_from_stock("gtk-save");
	g_signal_connect(button, "clicked", (GCallback)savedata, window);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), -1);

	objects = g_list_append(objects, data);

	gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 1);

	gtk_container_add (GTK_CONTAINER (scrolled), textview);
	*buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));

	gtk_widget_show_all (window);
	return window;
}

int main(int argc, char *argv[])
{
	GtkWidget *window;
	GtkWidget *hbox;
	GtkWidget *toolbar;
	GtkWidget *button;

	gtk_init (&argc, &argv);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_signal_connect ((GtkObject*)window, "destroy", GTK_SIGNAL_FUNC (gtk_main_quit), "WM destroy");
	gtk_window_set_decorated ((GtkWindow*)window, FALSE);
	gtk_widget_set_size_request(window, 320, -1);
	gtk_window_set_default_icon_name("gtk-execute");

	hbox = gtk_hbox_new (FALSE, 1);
	gtk_container_border_width (GTK_CONTAINER (hbox), 3);
	gtk_container_add (GTK_CONTAINER (window), hbox);

	toolbar = gtk_toolbar_new();

	button = (GtkWidget*)gtk_tool_button_new_from_stock("gtk-new");
	g_signal_connect(button, "clicked", (GCallback)newdata, NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), -1);

	button = (GtkWidget*)gtk_tool_button_new_from_stock("gtk-open");
	g_signal_connect(button, "clicked", (GCallback)opendata, NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), -1);

	gtk_box_pack_start (GTK_BOX (hbox), toolbar, TRUE, TRUE, 0);
	toolbar = gtk_toolbar_new();

	button = (GtkWidget*)gtk_tool_button_new_from_stock("gtk-add");
	g_signal_connect(button, "clicked", (GCallback)syspath, NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), -1);

	gtk_box_pack_start (GTK_BOX (hbox), toolbar, TRUE, TRUE, 1);
	toolbar = gtk_toolbar_new();

	button = (GtkWidget*)gtk_tool_button_new_from_stock("gtk-apply");
	g_signal_connect(button, "clicked", (GCallback)execute, NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), -1);

	gtk_box_pack_start (GTK_BOX (hbox), toolbar, TRUE, TRUE, 2);

	gtk_widget_show_all (window);
	gtk_main ();

	return(0);
}
