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
#include <X11/Xlib.h>
#include <cairo/cairo-xlib-xrender.h>
#include <math.h>
#include "dat.h"

extern GList *objects;

Display *dsp;
Pixmap pixmap;
Window rootwin;
int scrwidth;
int scrheight;
cairo_t *cr;

void arrowsinit() {
	dsp = XOpenDisplay(NULL);
	int scr = DefaultScreen(dsp);
	scrwidth = DisplayWidth(dsp, scr);
	scrheight = DisplayHeight(dsp, scr);

	rootwin = DefaultRootWindow(dsp);

	pixmap = XCreatePixmap(dsp, rootwin, scrwidth, scrheight, DefaultDepth(dsp, scr));
	XRenderPictFormat *xrformat = XRenderFindVisualFormat(dsp, DefaultVisual(dsp, scr)); 

	cairo_surface_t *surface = cairo_xlib_surface_create_with_xrender_format(dsp, pixmap, ScreenOfDisplay(dsp, scr), xrformat, scrwidth, scrheight);
	cr = cairo_create(surface);

	XSetWindowBackgroundPixmap(dsp, rootwin, pixmap);
}

void arrowsupdate() {
	GList *iter;
	Object *obj;
	gint x1, y1, x2, y2;
	double x3, y3, x4, y4;
	double theta;
	double dx, dy;
	int i, len, winwidth, winheight;

	cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
	cairo_rectangle(cr, 0, 0, scrwidth, scrheight);
	cairo_fill(cr);

	cairo_set_source_rgb(cr, 0.75, 0.1, 0.1);
	cairo_set_line_width(cr, 5.0);

	iter = objects;
	while (iter != NULL) {
		obj = iter->data;
		if (obj->prev != NULL) {
			gtk_window_get_position((GtkWindow*)obj->prev->window, &x1, &y1);
			gtk_window_get_size((GtkWindow*)obj->prev->window, &winwidth, &winheight);
			x1 += winwidth / 2;
			y1 += winheight / 2;
			gtk_window_get_position((GtkWindow*)obj->window, &x2, &y2);
			gtk_window_get_size((GtkWindow*)obj->window, &winwidth, &winheight);
			x2 += winwidth / 2;
			y2 += winheight / 2;

			cairo_move_to(cr, x2, y2);
			cairo_line_to(cr, x1, y1);

			dx = x2 - x1;
			dy = y2 - y1;
			len = sqrt(dx*dx + dy*dy);
			len /= 50;
			theta = atan2(dy, dx);
			dx = cos(theta) * 50;
			dy = sin(theta) * 50;
			x3 = x1 + dx;
			y3 = y1 + dy;

			for (i = 0; i != len; i++) {
				cairo_move_to(cr, x3, y3);

				x4 = x3 - 20 * cos(theta - M_PI/6);
				y4 = y3 - 20 * sin(theta - M_PI/6);
				cairo_line_to(cr, x4, y4);
				cairo_move_to(cr, x3, y3);

				x4 = x3 - 20 * cos(theta + M_PI/6);
				y4 = y3 - 20 * sin(theta + M_PI/6);
				cairo_line_to(cr, x4, y4);

				x3 += dx;
				y3 += dy;
			}

			cairo_stroke(cr);
		}
		iter = iter->next;
	}

	XClearWindow(dsp, rootwin);
	XMapRaised(dsp, rootwin);
	XFlush(dsp);
}

void arrowsdestroy() {
	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_rectangle(cr, 0, 0, scrwidth, scrheight);
	cairo_fill(cr);

	XClearWindow(dsp, rootwin);
	XMapRaised(dsp, rootwin);
	XFlush(dsp);

	XFreePixmap(dsp, pixmap);
}
