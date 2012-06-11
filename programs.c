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
	GtkWidget *menu, *menuitem, *toolitem;

	gtk_widget_add_events(program, GDK_CONFIGURE);
	g_signal_connect(program, "configure-event", (GCallback)winmove, NULL);
	g_signal_connect(program, "destroy", (GCallback)closeitem, data);
	gtk_window_set_default_size((GtkWindow*)program, 400, -1);

	GtkWidget *hbox = gtk_hbox_new (FALSE, 2);
	GtkWidget *toolbar = gtk_toolbar_new();
	GtkWidget *button = (GtkWidget*)gtk_tool_button_new_from_stock("gtk-connect");
	g_signal_connect(button, "clicked", (GCallback)connectobj, data);
	gtk_toolbar_insert((GtkToolbar*)toolbar, (GtkToolItem*)button, -1);
	gtk_box_pack_start ((GtkBox*)hbox, toolbar, TRUE, TRUE, 0);

	menu = gtk_menu_new();

	menuitem = (GtkWidget*)gtk_image_menu_item_new_from_stock("gtk-go-forward", NULL);
	gtk_menu_item_set_label((GtkMenuItem*)menuitem, "Once");
	g_signal_connect(menuitem, "activate", (GCallback)execute, data);
	gtk_menu_shell_append((GtkMenuShell*)menu, menuitem);
	gtk_widget_show(menuitem);

	menuitem = (GtkWidget*)gtk_image_menu_item_new_from_stock("gtk-jump-to", NULL);
	gtk_menu_item_set_label((GtkMenuItem*)menuitem, "Loop");
	g_signal_connect(menuitem, "activate", (GCallback)execloop, data);
	gtk_menu_shell_append((GtkMenuShell*)menu, menuitem);
	gtk_widget_show(menuitem);

	toolitem = (GtkWidget*)gtk_tool_button_new_from_stock("gtk-go-forward");
	gtk_tool_button_set_label((GtkToolButton*)toolitem, "Run"); 
	gtk_toolbar_insert((GtkToolbar*)toolbar, (GtkToolItem*)toolitem, -1);
	g_signal_connect(toolitem, "clicked", (GCallback)openmenu, (gpointer)menu);

	data->exec = toolitem;
	data->window = program;
	gtk_widget_set_sensitive(data->exec, FALSE);

	gtk_window_set_title((GtkWindow*)program, "Program");
	objects = g_list_append(objects, data);

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
					      &data->stdout, NULL, &error)) {
			if (!data->looping) {
				dialog = gtk_message_dialog_new(NULL, 0,
						GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
						"%s", error->message);
				gtk_dialog_run((GtkDialog*)dialog);
				gtk_widget_destroy(dialog);
			}

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
			buffer = gtk_text_view_get_buffer((GtkTextView*)data->data);
			gtk_text_buffer_get_start_iter(buffer, &start);
			gtk_text_buffer_get_end_iter(buffer, &end);
			termbuffer(data->prev->output, &buffer);
			gtk_text_view_set_buffer((GtkTextView*)data->data, buffer);
			gtk_text_buffer_get_start_iter(buffer, &start);
			gtk_text_buffer_get_end_iter(buffer, &end);
		// data from an input window
		} else if (data->prev->data != NULL) {
			buffer = gtk_text_view_get_buffer((GtkTextView*)data->prev->data);
			gtk_text_buffer_get_start_iter(buffer, &start);
			gtk_text_buffer_get_end_iter(buffer, &end);
			contents = gtk_text_buffer_get_text(buffer, &start, &end, TRUE);
			len = gtk_text_buffer_get_char_count(buffer);
			termbuffer(contents, &buffer);
			gtk_text_view_set_buffer((GtkTextView*)data->data, buffer);
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
	execute_r((Object*)obj);
}

gboolean execloopcb(gpointer obj) {
	execute(NULL, obj);

	return ((Object*)obj)->looping;
}

void execloopstart(GtkWidget *button, gpointer obj) {
	g_timeout_add((gint)gtk_adjustment_get_value(
	    ((Object*)obj)->exectimeout), (GSourceFunc)execloopcb, obj);
	gtk_widget_destroy((GtkWidget*)((Object*)obj)->exec);
}

void execloop(GtkWidget *button, gpointer obj) {
	((Object*)obj)->looping = !((Object*)obj)->looping;

	if (((Object*)obj)->looping) {	
		GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		GtkWidget *hbox = gtk_hbox_new (FALSE, 3);

		((Object*)obj)->exec = window;

		gtk_window_set_default_size((GtkWindow*)window, 300, -1);

		GtkWidget *label = gtk_label_new("Interval (ms)");
		gtk_box_pack_start ((GtkBox*)hbox, label, FALSE, TRUE, 0);

		GtkWidget *spin = gtk_spin_button_new_with_range(1, 1000000000, 1);
		((Object*)obj)->exectimeout = gtk_spin_button_get_adjustment((GtkSpinButton*)spin);
		gtk_adjustment_set_value(((Object*)obj)->exectimeout, 1000);
		gtk_box_pack_start ((GtkBox*)hbox, spin, TRUE, TRUE, 1);

		GtkWidget *toolbar = gtk_toolbar_new();
		GtkWidget *toolbutton = (GtkWidget*)gtk_tool_button_new_from_stock("gtk-go-forward");
		gtk_tool_button_set_label((GtkToolButton*)toolbutton, "Run");
		g_signal_connect(toolbutton, "clicked", (GCallback)execloopstart, obj);
		gtk_toolbar_insert((GtkToolbar*)toolbar, (GtkToolItem*)toolbutton, -1);
		gtk_box_pack_start ((GtkBox*)hbox, toolbar, TRUE, TRUE, 2);

		gtk_container_add ((GtkContainer*)window, hbox);
		gtk_widget_show_all(window);
	}
}

