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

void opendata(GtkWidget *button, gpointer __unused) {
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

void newdata(GtkWidget *button, gpointer __unused) {
	GtkTextBuffer *buffer;
	Object *data;
	GtkWidget *window = new_data_window(&buffer, &data);
	gtk_window_set_title(GTK_WINDOW(window), "[unsaved data]");
}

void savedata(GtkWidget *button, gpointer window) {
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

void undodata(GtkWidget *button, gpointer obj) {
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

void redodata(GtkWidget *button, gpointer obj) {
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

GtkWidget* new_data_window(GtkTextBuffer **textbuffer, Object **object) {
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

	gtk_widget_add_events(window, GDK_CONFIGURE);
	g_signal_connect(window, "configure-event", (GCallback)winmove, NULL);
	g_signal_connect(window, "destroy", (GCallback)closeitem, data);
	gtk_window_set_default_size((GtkWindow*)window, 320, 180);
	gtk_container_set_border_width ((GtkContainer*)vbox, 1);
	gtk_container_add ((GtkContainer*)window, vbox);

	toolbar = gtk_toolbar_new();

	button = (GtkWidget*)gtk_tool_button_new_from_stock("gtk-connect");
	g_signal_connect(button, "clicked", (GCallback)connectobj, data);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), -1);

	button = (GtkWidget*)gtk_tool_button_new_from_stock("gtk-save");
	g_signal_connect(button, "clicked", (GCallback)savedata, data);
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
	pango_font_description_set_size(style->font_desc, 14 * PANGO_SCALE);
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

