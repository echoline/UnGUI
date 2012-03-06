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

void connectobj(GtkWidget *button, gpointer window) {
	Object *data = window;
	GList *nexts;

	// cancel/clear
	if (data == selected) {
		gtk_drag_unhighlight(button);
		if (selected != NULL) {
			if (selected->prev != NULL) {
				selected->prev->nexts = g_list_remove(selected->prev->nexts, selected);
				selected->prev = NULL;
			}

			while (selected->nexts != NULL) {
				((Object*)selected->nexts->data)->prev = NULL;
				selected->nexts = g_list_remove(selected->nexts, selected->nexts->data);
			}

			arrowsupdate();
		}
		selected = NULL;
		selbutton = NULL;
	// link
	} else if (selbutton != NULL) {
		gtk_drag_unhighlight(selbutton);
		if (g_list_find(selected->nexts, data) == NULL) {
			selected->nexts = g_list_append(selected->nexts, data);
			if (data->prev != NULL)
				data->prev->nexts = g_list_remove(data->prev->nexts, data);
			data->prev = selected;
			arrowsupdate();
		}
		selected = NULL;
		selbutton = NULL;
	// initiate
	} else {
		gtk_drag_highlight(button);
		selected = data;
		selbutton = button;
	}
}

gboolean winmovedirty = FALSE;

gboolean winmovecb(gpointer __unused) {
	arrowsupdate();
	winmovedirty = FALSE;
	return FALSE;
}

gboolean winmove(GtkWindow *window, GdkEvent *event, gpointer data) {
	if (winmovedirty == FALSE) {
		g_timeout_add(10, (GSourceFunc)winmovecb, NULL);
		winmovedirty = TRUE;
	}

	return FALSE;
}

void closeitem(GtkWidget *__unused, gpointer obj) {
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

	if (data->cmd != NULL)
		free(data->cmd);

	free(data);

	arrowsupdate();
}

void termbuffer(gchar *contents, GtkTextBuffer **buffer) {
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

void openmenu(GtkWidget *button, gpointer menu) {
	gtk_menu_popup((GtkMenu*)menu, NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
}

void cleanup(gpointer __unused) {
	arrowsdestroy();
	gtk_main_quit();
}

