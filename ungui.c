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
#include <string.h>
#define MAXESCSEQLEN 16 

/* type representation of program and data items */

typedef struct _Object Object;
struct _Object {
	GtkWidget *window;
	GList *nexts;
	Object *prev;
	GtkWidget *data;
	GtkWidget *undo;
	GtkWidget *redo;
	char *cmd;
	char *output;
	int outlen;
	int stdin;
	int stdout;
	GList *undohist;
	GList *undoptr;
};
Object *selected = NULL;
GtkWidget *selbutton = NULL;
GList *objects = NULL;

static GtkWidget* new_data_window(GtkTextBuffer **, Object **);
static void closeitem(GtkWidget *__unused, gpointer);
static void execute(GtkWidget *button, gpointer program);

/* g_signal_connect callbacks */

static void connectobj(GtkWidget *button, gpointer window) {
	Object *data = window;
	GList *nexts;

	// cancel
	if (data == selected) {
		gtk_drag_unhighlight(button);
		if (selected != NULL) {
			if (selected->prev != NULL) {
				selected->prev->nexts = g_list_remove(selected->prev->nexts, selected);
				selected->prev = NULL;
			}

			while (selected->nexts != NULL) {
				selected->nexts->prev = NULL;
				selected->nexts = g_list_remove(selected->nexts, selected);
			}

			if (selected->cmd != NULL)
				free(selected->cmd);
		}
		selected = NULL;
		selbutton = NULL;
	// link
	} else if (selbutton != NULL) {
		gtk_drag_unhighlight(selbutton);
		selected->nexts = g_list_append(selected->nexts, data);
		if (data->prev != NULL)
			data->prev->nexts = g_list_remove(data->prev->nexts, data);
		data->prev = selected;
		selected = NULL;
		selbutton = NULL;
	// initiate
	} else {
		gtk_drag_highlight(button);
		selected = data;
		selbutton = button;
	}
}

static gboolean syspathmatch(GtkEntryCompletion *complete, const gchar *key, GtkTreeIter *iter, gpointer opts) {
	
}

static void syspath(GtkWidget *button, gpointer window) {
	GtkWidget *dialog = gtk_dialog_new_with_buttons("Select Utility", NULL, 0, NULL);
	GtkWidget *vbox = gtk_vbox_new(FALSE, 2);
	GtkEntry *text = (GtkEntry*)gtk_entry_new();
	GtkEntryCompletion *complete = gtk_entry_get_completion(text);
	gchar **path = g_strsplit(g_getenv("PATH"), ":", -1);

	gtk_entry_completion_set_match_func(complete, (GtkEntryCompletionMatchFunc)syspathmatch, NULL, NULL);

	gtk_box_pack_start ((GtkBox*)vbox, (GtkWidget*)text, TRUE, TRUE, 1);
	gtk_container_add((GtkContainer*)gtk_dialog_get_content_area((GtkDialog*)dialog), vbox);

	gtk_dialog_add_button((GtkDialog*)dialog, "gtk-cancel", GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button((GtkDialog*)dialog, "gtk-open", GTK_RESPONSE_ACCEPT);
	gtk_entry_set_activates_default(text, TRUE);
	gtk_dialog_set_default_response((GtkDialog*)dialog, GTK_RESPONSE_ACCEPT);

	gtk_widget_show_all(dialog);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		GtkWidget *program = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		const gchar *cmd = gtk_entry_get_text((GtkEntry*)text);
		Object *data = calloc(1, sizeof(Object));
		data->cmd = g_strdup(cmd);
		
		g_signal_connect(program, "destroy", (GCallback)closeitem, data);
		gtk_window_set_title(GTK_WINDOW(program), cmd);
		gtk_widget_set_size_request(program, 100, -1);

		GtkWidget *hbox = gtk_hbox_new (FALSE, 1);
		GtkWidget *toolbar = gtk_toolbar_new();

		data->window = program;
		objects = g_list_append(objects, data);

		GtkWidget *button = (GtkWidget*)gtk_tool_button_new_from_stock("gtk-connect");
		g_signal_connect(button, "clicked", (GCallback)connectobj, data);
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
	Object *data;
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
			window = new_data_window(&buffer, &data);
			gtk_window_set_title(GTK_WINDOW(window), filename);
			gtk_text_buffer_get_start_iter(buffer, &start);
			gtk_text_buffer_insert(buffer, &start, contents, -1);
			data->undohist = g_list_append(data->undohist, buffer);
			data->undoptr = g_list_last(data->undohist);
		}

		g_free (filename);
	}
	gtk_widget_destroy (dialog);
}

static void newdata(GtkWidget *button, gpointer __unused) {
	GtkTextBuffer *buffer;
	Object *data;
	GtkWidget *window = new_data_window(&buffer, &data);
	gtk_window_set_title(GTK_WINDOW(window), "[unsaved data]");
}

static void savedata(GtkWidget *button, gpointer window) {
	GtkWidget *dialog;
	GtkTextBuffer *buffer;
	char *filename;
	gchar *contents;
	GtkTextIter start, end;
	Object *data = window;

	if (data == NULL)
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
		contents = gtk_text_buffer_get_text(buffer, &start, &end, TRUE);
		g_file_set_contents(filename, contents, -1, NULL);
		data->undohist = g_list_append(data->undohist, buffer);
		data->undoptr = g_list_last(data->undohist);
		gtk_widget_set_sensitive(data->redo, FALSE);
		gtk_widget_set_sensitive(data->undo, TRUE);
		gtk_window_set_title(GTK_WINDOW(data->window), filename);
		g_free (filename);
	}
	gtk_widget_destroy (dialog);
}

static void closeitem(GtkWidget *__unused, gpointer obj) {
	Object *data = obj;
	GList *iter2;

	if (data == NULL)
		return;

	iter2 = objects;
	while (iter2 != NULL) {
		((Object*)iter2->data)->nexts = g_list_remove (((Object*)iter2->data)->nexts, data);
		if (((Object*)iter2->data)->prev == data)
			((Object*)iter2->data)->prev = NULL;
		iter2 = iter2->next;
	}

	objects = g_list_remove(objects, data);
	free(data);
}

static void termbuffer(gchar *contents, GtkTextBuffer **buffer) {
	gchar **sections = g_strsplit(contents, (char[]){ 0x1B, 0 }, -1);
	gint i = 0, j;
	GtkTextIter start, end;
	GtkTextTag *tag;
	gchar *content = NULL, *ptr, *tok;

	*buffer = gtk_text_buffer_new(NULL);
	gtk_text_buffer_get_start_iter(*buffer, &start);
	gtk_text_buffer_get_end_iter(*buffer, &end);
	gtk_text_buffer_delete(*buffer, &start, &end);
	gtk_text_buffer_get_start_iter(*buffer, &end); // meh.

	while (sections[i] != NULL) {
		tag = gtk_text_buffer_create_tag(*buffer, NULL, NULL);
		if (sections[i][0] != '[') {
			if (i != 0)
				content = g_strconcat((char[]){ 0x1B, 0 }, sections[i], NULL);
			else
				content = g_strconcat(sections[i], NULL);
			
		} else {
			ptr = &sections[i][1];

			if (strchr(ptr, 'm') != NULL) {
				ptr[strcspn(ptr, "m")] = '\0';
				j = strlen(ptr)+1;
				tok = strtok(ptr, ";");
				if (tok != NULL) do {
					switch(atoi(tok)) {
					case 0:
						g_object_set(tag, "weight", PANGO_WEIGHT_NORMAL, NULL);
						break;
					case 1:
						g_object_set(tag, "weight", PANGO_WEIGHT_BOLD, NULL);
						break;
					case 30:
						g_object_set(tag, "foreground", "black", NULL);
						break;
					case 31:
						g_object_set(tag, "foreground", "red", NULL);
						break;
					case 32:
						g_object_set(tag, "foreground", "darkgreen", NULL);
						break;
					case 33:
						g_object_set(tag, "foreground", "orange", NULL);
						break;
					case 34:
						g_object_set(tag, "foreground", "blue", NULL);
						break;
					case 35:
						g_object_set(tag, "foreground", "purple", NULL);
						break;
					case 36:
						g_object_set(tag, "foreground", "cyan", NULL);
						break;
					case 37:
						g_object_set(tag, "foreground", "white", NULL);
						break;
					case 40:
						g_object_set(tag, "background", "black", NULL);
						break;
					case 41:
						g_object_set(tag, "background", "red", NULL);
						break;
					case 42:
						g_object_set(tag, "background", "green", NULL);
						break;
					case 43:
						g_object_set(tag, "background", "yellow", NULL);
						break;
					case 44:
						g_object_set(tag, "background", "blue", NULL);
						break;
					case 45:
						g_object_set(tag, "background", "purple", NULL);
						break;
					case 46:
						g_object_set(tag, "background", "cyan", NULL);
						break;
					case 47:
						g_object_set(tag, "background", "white", NULL);
						break;
					}
				} while(tok = strtok(NULL, ";"));

				content = g_strconcat(&ptr[j], NULL);
			} else {
				content = g_strconcat(ptr, NULL);
			}
		}

		gtk_text_buffer_insert_with_tags(*buffer, &end, content, -1, tag, NULL);
		gtk_text_buffer_get_end_iter(*buffer, &end);
		g_free(content);
		i++;
	}

	g_strfreev(sections);
}

static void execute_r(Object *data) {
	GtkTextBuffer *buffer;
	GtkTextIter start, end;
	gint len;
	gchar *contents = NULL;
	char c;
	GError *error = NULL;
	GtkWidget *dialog;
	gint argc;
	char **argv;
	GList *iter = data->nexts;

	// destination is a program
	if (data->data == NULL) {
		if (!g_shell_parse_argv(data->cmd, &argc, &argv, NULL))
			return;

		if (!g_spawn_async_with_pipes(NULL, argv, NULL, 0, NULL,
					      NULL, NULL, &data->stdin,
					      &data->stdout, NULL,
					      &error))
		{
			dialog = gtk_message_dialog_new(NULL, 0,
					GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
					"%s", error->message);
			gtk_dialog_run((GtkDialog*)dialog);
			gtk_widget_destroy(dialog);
			return;
		}

		if (data->output == NULL) {
			data->outlen = 0;
			while (read(data->stdout, &c, 1) == 1) {
				data->outlen++;
				data->output = realloc(data->output, data->outlen+1);
				data->output[data->outlen-1] = c;
				data->output[data->outlen] = '\0';
			}
			close(data->stdout);
		}

		// previous object exists
		if (data->prev != NULL) {
			// and is a program
			if (data->prev->output != NULL) {
				write(data->stdin, data->prev->output, data->prev->outlen);
				close(data->stdin);
			// or is text data
			} else if (data->prev->data != NULL) {
				buffer = gtk_text_view_get_buffer((GtkTextView*)data->prev->data);
				len = gtk_text_buffer_get_char_count(buffer);
				gtk_text_buffer_get_start_iter(buffer, &start);
				gtk_text_buffer_get_end_iter(buffer, &end);
				contents = gtk_text_buffer_get_text(buffer, &start, &end, TRUE);
				write(data->stdin, contents, len);
				close(data->stdin);
			}
		}
	// data destination is an input window
	} else if (data->prev != NULL) {
		// from a program
		if (data->prev->output != NULL) {
			if (data->undohist == NULL)
				gtk_widget_set_sensitive(data->undo, FALSE);
			else
				gtk_widget_set_sensitive(data->undo, TRUE);
			gtk_widget_set_sensitive(data->redo, FALSE);

			buffer = gtk_text_view_get_buffer((GtkTextView*)data->data);
			gtk_text_buffer_get_start_iter(buffer, &start);
			gtk_text_buffer_get_end_iter(buffer, &end);
			data->undohist = g_list_append(data->undohist, buffer);
			termbuffer(data->prev->output, &buffer);
			gtk_text_view_set_buffer((GtkTextView*)data->data, buffer);
			gtk_text_buffer_get_start_iter(buffer, &start);
			gtk_text_buffer_get_end_iter(buffer, &end);
			data->undohist = g_list_append(data->undohist, buffer);
			data->undoptr = g_list_last(data->undohist);
		// data from an input window
		} else if (data->prev->data != NULL) {
			// store undo state
			buffer = gtk_text_view_get_buffer((GtkTextView*)data->data);
			gtk_text_buffer_get_start_iter(buffer, &start);
			gtk_text_buffer_get_end_iter(buffer, &end);
			contents = gtk_text_buffer_get_text(buffer, &start, &end, TRUE);
			if (data->undohist == NULL)
				gtk_widget_set_sensitive(data->undo, FALSE);
			else
				gtk_widget_set_sensitive(data->undo, TRUE);
			gtk_widget_set_sensitive(data->redo, FALSE);
			data->undohist = g_list_append(data->undohist, buffer);
			// fetch new undo state
			buffer = gtk_text_view_get_buffer((GtkTextView*)data->prev->data);
			gtk_text_buffer_get_start_iter(buffer, &start);
			gtk_text_buffer_get_end_iter(buffer, &end);
			contents = gtk_text_buffer_get_text(buffer, &start, &end, TRUE);
			len = gtk_text_buffer_get_char_count(buffer);
			termbuffer(contents, &buffer);
			gtk_text_view_set_buffer((GtkTextView*)data->data, buffer);
			data->undohist = g_list_append(data->undohist, buffer);
			data->undoptr = g_list_last(data->undohist);
		}
	}

	while (iter != NULL) {
		execute_r(iter->data);
		iter = iter->next;
	}

	if (data->output != NULL) {
		free(data->output);
		data->output = NULL;
	}

	if (data->data == NULL) {
		close(data->stdin);
		close(data->stdout);
	}
}

static void execute(GtkWidget *button, gpointer obj) {
	GList *list = objects;

	// for each start
	while (list != NULL) {
		// only start from "beginning" objects
		if (((Object*)list->data)->prev == NULL)
			execute_r(list->data);

		list = list->next;
	}
}

/* end of g_signal_connect callbacks */

static void undodata(GtkWidget *button, gpointer obj) {
	Object *data = obj;

	if (data->undoptr->prev != NULL) {
		data->undoptr = data->undoptr->prev;
		if (GTK_IS_TEXT_BUFFER(data->undoptr->data)) // eh??
			gtk_text_view_set_buffer((GtkTextView*)data->data, (GtkTextBuffer*)data->undoptr->data);
	}
	if (data->undoptr->prev != NULL)
		gtk_widget_set_sensitive(data->undo, TRUE);
	else
		gtk_widget_set_sensitive(data->undo, FALSE);
	if (data->undoptr->next != NULL)
		gtk_widget_set_sensitive(data->redo, TRUE);
	else
		gtk_widget_set_sensitive(data->redo, FALSE);
}

static void redodata(GtkWidget *button, gpointer obj) {
	Object *data = obj;

	if (data->undoptr->next != NULL) {
		data->undoptr = data->undoptr->next;
		gtk_text_view_set_buffer((GtkTextView*)data->data, (GtkTextBuffer*)data->undoptr->data);
	}
	if (data->undoptr->prev != NULL)
		gtk_widget_set_sensitive(data->undo, TRUE);
	else
		gtk_widget_set_sensitive(data->undo, FALSE);
	if (data->undoptr->next != NULL)
		gtk_widget_set_sensitive(data->redo, TRUE);
	else
		gtk_widget_set_sensitive(data->redo, FALSE);
}

static GtkWidget* new_data_window(GtkTextBuffer **textbuffer, Object **object) {
	GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	GtkWidget *vbox = gtk_vbox_new (FALSE, 2);
	GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
	GtkWidget *textview = gtk_text_view_new();
	GtkWidget *toolbar;
	GtkWidget *button;
	GtkTextBuffer *buffer;
	GtkStyle *style;

	Object *data = calloc(1, sizeof(Object));
	data->data = textview;
	data->window = window;

	g_signal_connect(window, "destroy", (GCallback)closeitem, data);
	gtk_widget_set_size_request(window, 320, 180);
	gtk_container_border_width (GTK_CONTAINER (vbox), 1);
	gtk_container_add (GTK_CONTAINER (window), vbox);

	toolbar = gtk_toolbar_new();

	button = (GtkWidget*)gtk_tool_button_new_from_stock("gtk-connect");
	g_signal_connect(button, "clicked", (GCallback)connectobj, data);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), -1);

	button = (GtkWidget*)gtk_tool_button_new_from_stock("gtk-save");
	g_signal_connect(button, "clicked", (GCallback)savedata, data);
	gtk_tool_button_set_label((GtkToolButton*)button, "Write");
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), -1);

	data->undo = button = (GtkWidget*)gtk_tool_button_new_from_stock("gtk-undo");
	g_signal_connect(button, "clicked", (GCallback)undodata, data);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), -1);

	data->redo = button = (GtkWidget*)gtk_tool_button_new_from_stock("gtk-redo");
	g_signal_connect(button, "clicked", (GCallback)redodata, data);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), -1);

	gtk_widget_set_sensitive(data->redo, FALSE);
	gtk_widget_set_sensitive(data->undo, FALSE);
	objects = g_list_append(objects, data);

	gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 1);

	style = gtk_widget_get_style(textview);
	pango_font_description_set_family(style->font_desc, "fixed");
	gtk_widget_modify_font(textview, style->font_desc);

	gtk_container_add (GTK_CONTAINER (scrolled), textview);
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
	if (textbuffer != NULL)
		*textbuffer = buffer;
	if (object != NULL)
		*object = data;

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
	gtk_window_set_default_icon_name("gtk-yes");

	hbox = gtk_hbox_new (FALSE, 3);
	gtk_container_border_width (GTK_CONTAINER (hbox), 3);
	gtk_container_add (GTK_CONTAINER (window), hbox);

	toolbar = gtk_toolbar_new();

	button = (GtkWidget*)gtk_tool_button_new_from_stock("gtk-execute");
	gtk_tool_button_set_label((GtkToolButton*)button, "Programs"); 
	g_signal_connect(button, "clicked", (GCallback)syspath, NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), -1);

	button = (GtkWidget*)gtk_tool_button_new_from_stock("gtk-new");
	g_signal_connect(button, "clicked", (GCallback)newdata, NULL);
	gtk_tool_button_set_label((GtkToolButton*)button, "Data");
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), -1);

	button = (GtkWidget*)gtk_tool_button_new_from_stock("gtk-open");
	g_signal_connect(button, "clicked", (GCallback)opendata, NULL);
	gtk_tool_button_set_label((GtkToolButton*)button, "Read");
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), -1);

	button = (GtkWidget*)gtk_tool_button_new_from_stock("gtk-go-forward");
	gtk_tool_button_set_label((GtkToolButton*)button, "Run"); 
	g_signal_connect(button, "clicked", (GCallback)execute, NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), -1);

	gtk_box_pack_start (GTK_BOX (hbox), toolbar, TRUE, TRUE, 2);

	gtk_widget_show_all (window);
	gtk_main ();

	return(0);
}
