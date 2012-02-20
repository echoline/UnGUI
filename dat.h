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
#define MAXESCSEQLEN 16 

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
