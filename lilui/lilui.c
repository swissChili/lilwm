#include "lilui.h"
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NEW(x) (calloc(sizeof(x), 1))
#define BUF_MAX_LEN 32

static ui_window_t g_windows[UI_MAX_WINDOWS];
static int g_numwindows;

void ui_init(ui_window_t *win)
{
	win->gc = DefaultGC(win->dpy, win->scr);
	XSetBackground(win->dpy, win->gc, RGB(36, 36, 48));
	XClearWindow(win->dpy, win->win);
}

void ui_start(ui_window_t *win)
{
	win->x = 0;
	win->y = 0;
}

void ui_setwindow(ui_window_t *win)
{
	XSetICFocus(win->ic);
	XMapWindow(win->dpy, win->win);
}

ui_window_t ui_window(int width, int height)
{
	ui_window_t win;
	Display *dpy = XOpenDisplay(NULL);
	int scr = DefaultScreen(dpy);
	Window w =
		XCreateSimpleWindow(dpy, RootWindow(dpy, scr), 10, 10, width, height, 1,
							BlackPixel(dpy, scr), WhitePixel(dpy, scr));
	if (dpy == NULL)
	{
		fprintf(stderr, "Could not open X display\n");
		exit(EXIT_FAILURE);
	}
	XSelectInput(dpy, w, ExposureMask | KeyPressMask | ButtonPressMask);
	win.dpy = dpy;
	win.scr = scr;
	win.win = w;
	win.row.len = 0;
	win.buflen = 0;

	win.im = XOpenIM(dpy, NULL, NULL, NULL);
	win.ic =
		XCreateIC(win.im, XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
				  XNClientWindow, w, NULL);

	g_windows[g_numwindows++] = win;

	return win;
}

void ui_deletewindow(ui_window_t win)
{
}

void ui_fg(ui_window_t *win, unsigned long color)
{
	XSetForeground(win->dpy, win->gc, color);
}

void ui_fg3(ui_window_t *win, unsigned char r, unsigned char g, unsigned char b)
{
	XSetForeground(win->dpy, win->gc, RGB(r, g, b));
}

void ui_row(ui_window_t *win)
{
	for (int i = 0; i < win->row.len; i++)
	{
		if (win->row.wdgts[i].data && win->row.wdgts[i].del)
		{
			win->row.wdgts[i].del(win->row.wdgts[i].data);
		}
	}
	win->row.len = 0;
}

ui_widget_t ui_dynsz(ui_window_t *win, ui_widget_t w, int fw, int fh,
					 int *max_h)
{
	w.x = win->x;
	w.y = win->y;

	if (w.w < 0)
		w.w = fw * (-w.w);

	if (w.h < 0)
		w.h = fh * (-w.h);

	*max_h = MAX(*max_h, w.h);

	win->x += w.w;
	return w;
}

int ui_widgetclicked(ui_window_t *win, int i)
{
	if (i >= win->row.len)
		return 0;
	return ui_isclicked(win, win->row.wdgts[i]);
}

int ui_isclicked(ui_window_t *win, ui_widget_t w)
{
	int l = w.x, r = w.x + w.w, t = w.y, b = w.y + w.h;

	if (win->evt.type)
		printf("evt %d at %d, %d\n", win->evt.type, win->evt.x, win->evt.y);
	// printf("l r t b %d %d %d %d\n", l, r, t, b);

	if (win->evt.type == UI_EVT_CLICK)
	{
		return win->evt.x >= l && win->evt.x <= r && win->evt.y >= t &&
			   win->evt.y <= b;
	}
	else
		return 0;
}

int ui_clickedoff(ui_window_t *win, ui_widget_t w)
{
	if (!win->evt.type)
		return 0;

	return !ui_isclicked(win, w);
}

void ui_pack(ui_window_t *win)
{
	XWindowAttributes a;
	XGetWindowAttributes(win->dpy, win->win, &a);
	int w = a.width, h = a.height,
		w_w = 0,		// combined width of all widgets
		w_h = 0,		// combined height of all widgets
		flex_num_w = 0, // proportional flexible space
		flex_num_h = 0, max_h = 0;

	for (int i = 0; i < win->row.len; i++)
	{
		if (win->row.wdgts[i].w >= 0)
			w_w += win->row.wdgts[i].w;
		else
			flex_num_w += 0 - win->row.wdgts[i].w;

		if (win->row.wdgts[i].h >= 0)
			w_h += win->row.wdgts[i].h;
		else
			flex_num_h += 0 - win->row.wdgts[i].h;
	}
	// flexible space
	int flex_w = flex_num_w ? (w - w_w) / flex_num_w : 0,
		flex_h = flex_num_h ? (h - w_h) / flex_num_h : 0;

	for (int i = 0; i < win->row.len; i++)
	{
		win->row.wdgts[i] =
			ui_dynsz(win, win->row.wdgts[i], flex_w, flex_h, &max_h);
		win->row.wdgts[i].bld(win, win->row.wdgts[i]);
	}

	win->x = 0;
	win->y += max_h;
	// printf("Packed, y is now %d\n", g_y);
}

int ui_add(ui_window_t *win, ui_widget_t w)
{
	win->row.wdgts[win->row.len] = w;
	return win->row.len++;
}

void ui_clear(ui_window_t *win, unsigned long color)
{
	ui_fg(win, color);
	XClearWindow(win->dpy, win->win);
}

void ui_redraw(ui_window_t *win, ui_rendererloop_t rl)
{
	do
	{
		ui_clear(win, RGB(255, 255, 255));
		win->should_update = 0;
		ui_start(win);
		rl(win);
		win->evt.type = UI_EVT_NONE;
	} while (win->should_update);
}

int ui_windowevent(XEvent e, ui_window_t *win, ui_rendererloop_t rl)
{
	if (e.type == Expose)
	{
		if (e.xexpose.window != win->win)
		{
			return 0;
		}
		ui_redraw(win, rl);
	}
	else if (e.type == MappingNotify)
	{
		XRefreshKeyboardMapping(&e.xmapping);
	}
	else if (e.type == KeyPress)
	{
		if (e.xkey.window != win->win)
		{
			return 0;
		}

		Status status = 0;
		win->buflen = Xutf8LookupString(win->ic, (XKeyPressedEvent *)&e,
										win->buf, 20, &win->keysym, &status);

		char *keystr = XKeysymToString(win->keysym);

		if (status == XBufferOverflow)
		{
			printf("Buffer overflow");
		}
		if (status == XLookupKeySym || status == XLookupBoth)
		{
			printf("Status: %d\n", status);
		}
		printf("Pressed key %s (%lu)\n", keystr, win->keysym);

		ui_redraw(win, rl);
	}
	else if (e.type == ButtonPress)
	{
		if (e.xbutton.window != win->win)
		{
			return 0;
		}

		if (e.xbutton.button >= 1 && e.xbutton.button <= 3)
		{
			printf("Mouse clicked %d\n", e.xbutton.button);
			int x = e.xbutton.x, y = e.xbutton.y;
			win->evt = (ui_mouseevent_t){
				.evt = UI_MOUSE_DOWN,
				.type = e.xbutton.button,
				.x = x,
				.y = y,
			};

			ui_redraw(win, rl);
		}
	}
	else if (e.type == ButtonRelease)
	{
		if (e.xbutton.window != win->win)
		{
			return 0;
		}

		int x = e.xbutton.x, y = e.xbutton.y;
		win->evt = (ui_mouseevent_t){
			.evt = UI_MOUSE_UP,
			.type = e.xbutton.button,
			.x = x,
			.y = y,
		};

		ui_redraw(win, rl);
	}
	else
	{
		return 1;
	}
	return 0;
}

void ui_loop(ui_rendererloop_t rl)
{
	XEvent e;
	for (int i = 0; i < g_numwindows; i++)
	{
		ui_init(&g_windows[i]);
	}
	while (1)
	{
		for (int i = 0; i < g_numwindows; i++)
		{
			ui_window_t *win = &g_windows[i];

			win->evt.type = UI_EVT_NONE;

			win->buflen = 0;
			win->keysym = 0;

			XNextEvent(win->dpy, &e);
			if (XFilterEvent(&e, win->win))
				continue;

			if (ui_windowevent(e, win, rl))
				break;
		}
	}
	// TODO: Close displays (or even better, only open one)
	// XCloseDisplay(win->dpy);
}
