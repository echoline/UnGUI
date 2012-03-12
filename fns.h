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

GtkWidget* new_data_window(GtkTextBuffer **, Object **);
void closeitem(GtkWidget *__unused, gpointer);
void execute(GtkWidget *button, gpointer program);
void execloop(GtkWidget *button, gpointer program);
void cleanup(gpointer __unused);
void syspath(GtkWidget *button, gpointer window);
void newdata(GtkWidget *button, gpointer __unused);
void opendata(GtkWidget *button, gpointer __unused);
void openmenu(GtkWidget *button, gpointer menu);
gboolean winmove(GtkWindow *window, GdkEvent *event, gpointer data);
void connectobj(GtkWidget *button, gpointer window);
void arrowsinit();

