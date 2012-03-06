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
#include "dat.h"
#include "fns.h"

extern Object *selected;
extern GtkWidget *selbutton;
extern GList *objects;

gboolean cmdtextchanged(GtkWidget *text, GdkEvent *__unused, gpointer data) {
	Object *obj = data;

	if (obj->cmd != NULL)
		free(obj->cmd);

	obj->cmd = g_strdup(gtk_entry_get_text((GtkEntry*)text));

	return FALSE;
}

void syspath(GtkWidget *__unused, gpointer window) {
	GtkWidget *program = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	Object *data = calloc(1, sizeof(Object));

	gtk_widget_add_events(program, GDK_CONFIGURE);
	g_signal_connect(program, "configure-event", (GCallback)winmove, NULL);
	g_signal_connect(program, "destroy", (GCallback)closeitem, data);
	gtk_window_set_default_size((GtkWindow*)program, 320, -1);

	GtkWidget *hbox = gtk_hbox_new (FALSE, 2);
	GtkWidget *toolbar = gtk_toolbar_new();

	data->window = program;
	gtk_window_set_title((GtkWindow*)program, "Program");
	objects = g_list_append(objects, data);

	GtkWidget *button = (GtkWidget*)gtk_tool_button_new_from_stock("gtk-connect");
	g_signal_connect(button, "clicked", (GCallback)connectobj, data);
	gtk_toolbar_insert((GtkToolbar*)toolbar, (GtkToolItem*)button, -1);
	gtk_box_pack_start ((GtkBox*)hbox, toolbar, TRUE, TRUE, 0);

	GtkWidget *text = gtk_entry_new();
	g_signal_connect(text, "focus-out-event", (GCallback)cmdtextchanged, data);
	gtk_box_pack_start ((GtkBox*)hbox, text, TRUE, TRUE, 1);

	gtk_container_add ((GtkContainer*)program, hbox);
	gtk_widget_show_all(program);
}

void execute_r(Object *data) {
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

	// this Object is a program
	if (data->data == NULL) {
		if (!g_shell_parse_argv(data->cmd, &argc, &argv, NULL))
			return;

		if (!g_spawn_async_with_pipes(NULL, argv, NULL, 0, NULL,
					      NULL, NULL, &data->stdin,
					      &data->stdout, NULL, &error))
		{
			dialog = gtk_message_dialog_new(NULL, 0,
					GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
					"%s", error->message);
			gtk_dialog_run((GtkDialog*)dialog);
			gtk_widget_destroy(dialog);
			return;
		}

		// has a previous Object
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
		} else {
			close(data->stdin);
		}

		if (data->output != NULL)
			free(data->output);

		data->outlen = 0;
		while (read(data->stdout, &c, 1) == 1) {
			data->outlen++;
			data->output = realloc(data->output, data->outlen+1);
			data->output[data->outlen-1] = c;
		}
		if (data->outlen != 0)
			data->output[data->outlen] = '\0';
		close(data->stdout);
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
}

void execute(GtkWidget *button, gpointer obj) {
	GList *list = objects;

	// for each start
	while (list != NULL) {
		// only start from "beginning" objects
		if (((Object*)list->data)->prev == NULL)
			execute_r(list->data);

		list = list->next;
	}
}

