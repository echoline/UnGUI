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

Object *selected = NULL;
GtkWidget *selbutton = NULL;
GList *objects = NULL;

int main(int argc, char *argv[])
{
	GtkWidget *window;
	GtkWidget *hbox;
	GtkWidget *toolbar;
	GtkWidget *toolitem;
	GtkWidget *menu;
	GtkWidget *menuitem;

	gtk_init (&argc, &argv);

	arrowsinit();
	arrowsupdate();

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	g_signal_connect (window, "destroy", (GCallback)cleanup, NULL);
	gtk_widget_set_size_request(window, 240, -1);
	gtk_window_set_resizable((GtkWindow*)window, FALSE);
	gtk_window_set_default_icon_name("gtk-yes");

	hbox = gtk_hbox_new (FALSE, 3);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 3);
	gtk_container_add (GTK_CONTAINER (window), hbox);

	toolbar = gtk_toolbar_new();

	toolitem = (GtkWidget*)gtk_tool_button_new_from_stock("gtk-execute");
	gtk_tool_button_set_label((GtkToolButton*)toolitem, "Programs"); 
	g_signal_connect(toolitem, "clicked", (GCallback)syspath, NULL);
	gtk_toolbar_insert((GtkToolbar*)toolbar, (GtkToolItem*)toolitem, -1);

	toolitem = (GtkWidget*)gtk_tool_button_new_from_stock("gtk-dnd");
	gtk_tool_button_set_label((GtkToolButton*)toolitem, "Data");
	gtk_toolbar_insert((GtkToolbar*)toolbar, (GtkToolItem*)toolitem, -1);

	menu = gtk_menu_new();

	menuitem = (GtkWidget*)gtk_image_menu_item_new_from_stock("gtk-new", NULL);
	g_signal_connect(menuitem, "activate", (GCallback)newdata, NULL);
	gtk_menu_shell_append((GtkMenuShell*)menu, menuitem);
	gtk_widget_show(menuitem);

	menuitem = (GtkWidget*)gtk_image_menu_item_new_from_stock("gtk-open", NULL);
	g_signal_connect(menuitem, "activate", (GCallback)opendata, NULL);
	gtk_menu_shell_append((GtkMenuShell*)menu, menuitem);
	gtk_widget_show(menuitem);

	g_signal_connect(toolitem, "clicked", (GCallback)openmenu, (gpointer)menu);

	toolitem = (GtkWidget*)gtk_tool_button_new_from_stock("gtk-go-forward");
	gtk_tool_button_set_label((GtkToolButton*)toolitem, "Run"); 
	g_signal_connect(toolitem, "clicked", (GCallback)execute, NULL);
	gtk_toolbar_insert((GtkToolbar*)toolbar, (GtkToolItem*)toolitem, -1);

	gtk_box_pack_start (GTK_BOX (hbox), toolbar, TRUE, TRUE, 2);

	gtk_widget_show_all (window);
	gtk_main ();

	return(0);
}
