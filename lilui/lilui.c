#include "lilui.h"
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NEW(x) (calloc(sizeof(x), 1))
#define BUF_MAX_LEN 32

static ui_window_t *g_win;

// buffer pointer
static char g_buf[BUF_MAX_LEN];
static int g_buflen = 0;
static KeySym g_keysym = 0;

static ui_mouseevent_t g_evt;

void ui_init()
{
	g_win->gc = DefaultGC(g_win->dpy, g_win->scr);
	XSetBackground(g_win->dpy, g_win->gc, RGB(36, 36, 48));
	XClearWindow(g_win->dpy, g_win->win);
}

void ui_start()
{
	g_win->x = 0;
	g_win->y = 0;
}

void ui_setwindow(ui_window_t *win)
{
	g_win = win;
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

	win.im = XOpenIM(dpy, NULL, NULL, NULL);
	win.ic =
		XCreateIC(win.im, XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
				  XNClientWindow, w, NULL);

	return win;
}

void ui_deletewindow(ui_window_t win)
{

}

void ui_fg(unsigned long color)
{
	XSetForeground(g_win->dpy, g_win->gc, color);
}

void ui_fg3(unsigned char r, unsigned char g, unsigned char b)
{
	XSetForeground(g_win->dpy, g_win->gc, RGB(r, g, b));
}

void ui_row()
{
	for (int i = 0; i < g_win->row.len; i++)
	{
		if (g_win->row.wdgts[i].data && g_win->row.wdgts[i].del)
		{
			g_win->row.wdgts[i].del(g_win->row.wdgts[i].data);
		}
	}
	g_win->row.len = 0;
}

ui_widget_t ui_dynsz(ui_widget_t w, int fw, int fh, int *max_h)
{
	w.x = g_win->x;
	w.y = g_win->y;

	if (w.w < 0)
		w.w = fw * (-w.w);

	if (w.h < 0)
		w.h = fh * (-w.h);

	*max_h = MAX(*max_h, w.h);

	g_win->x += w.w;
	return w;
}

int ui_widgetclicked(int i)
{
	if (i >= g_win->row.len)
		return 0;
	return ui_isclicked(g_win->row.wdgts[i]);
}

int ui_isclicked(ui_widget_t w)
{
	int l = w.x, r = w.x + w.w, t = w.y, b = w.y + w.h;

	if (g_evt.type)
		printf("evt %d at %d, %d\n", g_evt.type, g_evt.x, g_evt.y);
	// printf("l r t b %d %d %d %d\n", l, r, t, b);

	if (g_evt.type == UI_EVT_CLICK)
	{
		return g_evt.x >= l && g_evt.x <= r && g_evt.y >= t && g_evt.y <= b;
	}
	else
		return 0;
}

int ui_clickedoff(ui_widget_t w)
{
	if (!g_evt.type)
		return 0;

	return !ui_isclicked(w);
}

void ui_pack()
{
	XWindowAttributes a;
	XGetWindowAttributes(g_win->dpy, g_win->win, &a);
	int w = a.width, h = a.height,
		w_w = 0,		// combined width of all widgets
		w_h = 0,		// combined height of all widgets
		flex_num_w = 0, // proportional flexible space
		flex_num_h = 0, max_h = 0;

	for (int i = 0; i < g_win->row.len; i++)
	{
		if (g_win->row.wdgts[i].w >= 0)
			w_w += g_win->row.wdgts[i].w;
		else
			flex_num_w += 0 - g_win->row.wdgts[i].w;

		if (g_win->row.wdgts[i].h >= 0)
			w_h += g_win->row.wdgts[i].h;
		else
			flex_num_h += 0 - g_win->row.wdgts[i].h;
	}
	// flexible space
	int flex_w = flex_num_w ? (w - w_w) / flex_num_w : 0,
		flex_h = flex_num_h ? (h - w_h) / flex_num_h : 0;

	for (int i = 0; i < g_win->row.len; i++)
	{
		g_win->row.wdgts[i] = ui_dynsz(g_win->row.wdgts[i], flex_w, flex_h, &max_h);
		g_win->row.wdgts[i].bld(g_win, g_win->row.wdgts[i]);
	}

	g_win->x = 0;
	g_win->y += max_h;
	// printf("Packed, y is now %d\n", g_y);
}

int ui_add(ui_widget_t w)
{
	g_win->row.wdgts[g_win->row.len] = w;
	return g_win->row.len++;
}

void ui_clear(unsigned long color)
{
	ui_fg(color);
	XClearWindow(g_win->dpy, g_win->win);
}

void ui_redraw(ui_rendererloop_t rl)
{
	ui_ctx_t ctx = {0};
	do
	{
		ui_clear(RGB(255, 255, 255));
		ctx.should_update = 0;
		ui_start();
		rl(&ctx);
		g_evt.type = UI_EVT_NONE;
	} while (ctx.should_update);
}

int ui_windowevent(XEvent e, ui_window_t *win, ui_rendererloop_t rl)
{
	if (e.type == Expose)
	{
		ui_redraw(rl);
	}
	else if (e.type == MappingNotify)
	{
		XRefreshKeyboardMapping(&e.xmapping);
	}
	else if (e.type == KeyPress)
	{
		Status status = 0;
		g_buflen = Xutf8LookupString(win->ic, (XKeyPressedEvent *)&e, g_buf,
									 20, &g_keysym, &status);

		char *keystr = XKeysymToString(g_keysym);

		if (status == XBufferOverflow)
		{
			printf("Buffer overflow");
		}
		if (g_buflen)
		{
			// printf("Buffer %.*s\n", g_buflen, g_buf);
		}
		if (status == XLookupKeySym || status == XLookupBoth)
		{
			printf("Status: %d\n", status);
		}
		printf("Pressed key %s (%lu)\n", keystr, g_keysym);

		ui_redraw(rl);
	}
	else if (e.type == ButtonPress)
	{
		if (e.xbutton.button >= 1 && e.xbutton.button <= 3)
		{
			printf("Mouse clicked %d\n", e.xbutton.button);
			int x = e.xbutton.x, y = e.xbutton.y;
			g_evt = (ui_mouseevent_t){
				.evt = UI_MOUSE_DOWN,
				.type = e.xbutton.button,
				.x = x,
				.y = y,
			};

			ui_redraw(rl);
		}
	}
	else if (e.type == ButtonRelease)
	{
		int x = e.xbutton.x, y = e.xbutton.y;
		g_evt = (ui_mouseevent_t){
			.evt = UI_MOUSE_UP,
			.type = e.xbutton.button,
			.x = x,
			.y = y,
		};

		ui_redraw(rl);
	} else { return 1; }
	return 0;
}

void ui_loop(ui_rendererloop_t rl)
{
	ui_init();
	XEvent e;
	while (1)
	{
		g_evt.type = UI_EVT_NONE;

		g_buflen = 0;
		g_keysym = 0;

		XNextEvent(g_win->dpy, &e);
		if (XFilterEvent(&e, g_win->win))
			continue;

		if (ui_windowevent(e, g_win, rl)) break;
	}
	XCloseDisplay(g_win->dpy);
}

void ui_bldrect(ui_window_t *win, ui_widget_t w)
{
	ui_fg(w.color);
	XFillRectangle(win->dpy, win->win, win->gc, w.x, w.y, w.w, w.h);
}

ui_widget_t ui_rect(int w, int h)
{
	return (ui_widget_t){
		.x = -1,
		.y = -1,
		.w = w,
		.h = h,
		.color = RGB(0, 0, 20),
		.bld = ui_bldrect,
	};
}

ui_widget_t ui_rect4(int x, int y, int w, int h)
{
	return (ui_widget_t){
		.x = x,
		.y = y,
		.w = w,
		.h = h,
		.color = RGB(0, 0, 20),
		.bld = ui_bldrect,
	};
}

void ui_bldtext(ui_window_t *win, ui_widget_t t)
{
	ui_fg(t.color);
	char *s = t.data;
	// printf("Drawing text %s at %d, %d\n", t.data, t.x, t.y);
	XDrawString(win->dpy, win->win, win->gc, t.x, t.y + 20, s, strlen(s));
}

ui_widget_t ui_text(char *text)
{
	return (ui_widget_t){
		.x = -1,
		.y = -1,
		.w = strlen(text) * 6, // TODO: make less shit
		.h = 32,
		.color = RGB(0, 0, 0),
		.data = (void *)text,
		.bld = ui_bldtext,
	};
}

void ui_bldbtn(ui_window_t *win, ui_widget_t b)
{
	b.color = RGB(50, 154, 229);
	ui_bldrect(win, b);
	b.x += 8;
	b.color = RGB(255, 255, 255);
	ui_bldtext(win, b);
}

ui_widget_t ui_btn(char *text)
{
	ui_widget_t t = ui_text(text);
	t.w += 16;
	t.bld = ui_bldbtn;
	return t;
}

void ui_bldnothing(ui_window_t *win, ui_widget_t w)
{
	// do nothing!
}

ui_widget_t ui_hspacer(int size)
{
	return (ui_widget_t){
		.x = -1, .y = -1, .w = size, .h = 1, .bld = ui_bldnothing};
}

ui_widget_t ui_vspacer(int size)
{
	return (ui_widget_t){
		.x = -1, .y = -1, .w = 1, .h = size, .bld = ui_bldnothing};
}

void ui_bldinputstr(ui_window_t *win, ui_widget_t i)
{
	ui_inputstr_data_t *d = (ui_inputstr_data_t *)i.data;
	printf("Currnet focus is %d\n", d->focused);
	printf("keysym: %d, Left: %d\n", g_keysym, XK_Left);
	if (ui_isclicked(i))
	{
		printf("Focused ui_inputstr\n");
		d->focused = 1;
	}
	if (ui_clickedoff(i))
	{
		printf("Clicked off ui_inputstr\n");
		d->focused = 0;
	}
	if (d->focused)
	{
		if (g_keysym == XK_BackSpace)
		{
			if (strlen(d->text) > 0)
			{
				memmove(&d->text[d->cursor - 1], &d->text[d->cursor],
						strlen(d->text) - (d->cursor - 1));
				d->cursor--;
			}
		}
		else if (g_keysym == XK_Delete)
		{
			printf("Deleting\n");
			memmove(&d->text[d->cursor], &d->text[d->cursor + 1],
					strlen(d->text) - d->cursor);
		}
		else if (g_keysym == XK_Left)
		{
			printf("Cursor Left %d\n", d->cursor);
			if (d->cursor > 0)
				d->cursor--;
		}
		else if (g_keysym == XK_Right)
		{
			if (d->cursor < strlen(d->text))
				d->cursor++;
		}
		else if (g_keysym == XK_Up)
		{
			d->cursor = 0;
		}
		else if (g_keysym == XK_Down)
		{
			d->cursor = strlen(d->text);
		}
		else if (g_buflen && d->len > strlen(d->text) + g_buflen)
		{
			printf("appending to buffer %.*s\n", g_buflen, g_buf);
			g_buf[g_buflen] = 0;
			printf("buf is %s, %s\n", g_buf, d->text);
			strncat(d->text, g_buf, g_buflen);
			d->cursor += g_buflen;
		}
		else
			printf("ui_inputstr buffer overflow\n");
	}

	puts("setting rect color");
	i.color = RGB(220, 250, 250);
	ui_bldrect(win, i);
	i.x += 8;
	i.color = RGB(0, 0, 0);
	i.data = d->text;
	ui_bldtext(win, i);
	// draw a little cursor
	if (d->focused)
	{
		ui_widget_t cursorw = ui_rect4(i.x + 6 * d->cursor, i.y + 10, 1, 12);
		cursorw.color = RGB(0, 0, 0);
		cursorw.bld(win, cursorw);
	}
}

ui_widget_t ui_inputstr(ui_inputstr_data_t *data, int wlen)
{
	return (ui_widget_t){
		.x = -1,
		.y = -1,
		.w = wlen,
		.h = 32,
		.bld = ui_bldinputstr,
		.data = data,
	};
}
