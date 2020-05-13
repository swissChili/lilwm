#include "lilui.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <X11/keysym.h>

#define NEW(x) (calloc(sizeof(x), 1))
#define BUF_MAX_LEN 32

static ui_window_t g_win;
static Display *g_d;
static Window g_w;
static int g_s;
static GC g_gc;
static ui_row_t g_row;

static int g_x = 0;
static int g_y = 0;

// buffer pointer
static char g_buf[BUF_MAX_LEN];
static int g_buflen = 0;
static KeySym g_keysym = 0;

static ui_mouseevent_t g_evt;

void ui_init()
{
	g_gc = DefaultGC(g_d, g_s);
	XSetBackground(g_d, g_gc, RGB(36, 36, 48));
	XClearWindow(g_d, g_w);
}

void ui_start()
{
	g_x = 0;
	g_y = 0;
}

void ui_setwindow(ui_window_t win)
{
	g_w = win.win;
	g_d = win.dpy;
	g_s = win.scr;
	g_win = win;
	XSetICFocus(win.ic);
	XMapWindow(g_d, g_w);
}

ui_window_t ui_window()
{
	ui_window_t win;
	Display *dpy = XOpenDisplay(NULL);
	int scr = DefaultScreen(dpy);
	Window w =
		XCreateSimpleWindow(dpy, RootWindow(dpy, scr), 10, 10, 100, 100, 1,
							BlackPixel(dpy, scr), WhitePixel(dpy, scr));
	if (dpy == NULL)
	{
		fprintf(stderr, "Could not open X display\n");
	}
	XSelectInput(dpy, w, ExposureMask | KeyPressMask | ButtonPressMask);
	win.dpy = dpy;
	win.scr = scr;
	win.win = w;

	win.im = XOpenIM(dpy, NULL, NULL, NULL);
	win.ic =
		XCreateIC(win.im, XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
				  XNClientWindow, w, NULL);

	return win;
}

void ui_fg(unsigned long color)
{
	XSetForeground(g_d, g_gc, color);
}

void ui_fg3(unsigned char r, unsigned char g, unsigned char b)
{
	XSetForeground(g_d, g_gc, RGB(r, g, b));
}

void ui_row()
{
	for (int i = 0; i < g_row.len; i++)
	{
		if (g_row.wdgts[i].data && g_row.wdgts[i].del)
		{
			g_row.wdgts[i].del(g_row.wdgts[i].data);
		}
	}
	g_row.len = 0;
}

ui_widget_t ui_dynsz(ui_widget_t w, int fw, int fh, int *max_h)
{
	w.x = g_x;
	w.y = g_y;

	if (w.w < 0)
		w.w = fw * (-w.w);

	if (w.h < 0)
		w.h = fh * (-w.h);

	*max_h = MAX(*max_h, w.h);

	g_x += w.w;
	return w;
}

int ui_widgetclicked(int i)
{
	if (i >= g_row.len)
		return 0;
	return ui_isclicked(g_row.wdgts[i]);
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
	XGetWindowAttributes(g_d, g_w, &a);
	int w = a.width, h = a.height,
		w_w = 0,		// combined width of all widgets
		w_h = 0,		// combined height of all widgets
		flex_num_w = 0, // proportional flexible space
		flex_num_h = 0, max_h = 0;

	for (int i = 0; i < g_row.len; i++)
	{
		if (g_row.wdgts[i].w >= 0)
			w_w += g_row.wdgts[i].w;
		else
			flex_num_w += 0 - g_row.wdgts[i].w;

		if (g_row.wdgts[i].h >= 0)
			w_h += g_row.wdgts[i].h;
		else
			flex_num_h += 0 - g_row.wdgts[i].h;
	}
	// flexible space
	int flex_w = flex_num_w ? (w - w_w) / flex_num_w : 0,
		flex_h = flex_num_h ? (h - w_h) / flex_num_h : 0;

	for (int i = 0; i < g_row.len; i++)
	{
		g_row.wdgts[i] = ui_dynsz(g_row.wdgts[i], flex_w, flex_h, &max_h);
		g_row.wdgts[i].bld(g_row.wdgts[i]);
	}

	g_x = 0;
	g_y += max_h;
	// printf("Packed, y is now %d\n", g_y);
}

int ui_add(ui_widget_t w)
{
	g_row.wdgts[g_row.len] = w;
	return g_row.len++;
}

void ui_clear(unsigned long color)
{
	ui_fg(color);
	XClearWindow(g_d, g_w);
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

void ui_loop(ui_rendererloop_t rl)
{
	ui_init();
	XEvent e;
	while (1)
	{
		g_evt.type = UI_EVT_NONE;

		g_buflen = 0;
		g_keysym = 0;

		XNextEvent(g_d, &e);
		if (XFilterEvent(&e, g_w))
			continue;

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
			g_buflen = Xutf8LookupString(g_win.ic, (XKeyPressedEvent*)&e, g_buf, 20, &g_keysym, &status);

			char *keystr = XKeysymToString(g_keysym);

			if (status == XBufferOverflow)
			{
				printf("Buffer overflow");
			}
			if (g_buflen)
			{
				//printf("Buffer %.*s\n", g_buflen, g_buf);
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
		}
	}
	XCloseDisplay(g_d);
}

void ui_bldrect(ui_widget_t w)
{
	ui_fg(w.color);
	XFillRectangle(g_d, g_w, g_gc, w.x, w.y, w.w, w.h);
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

void ui_bldtext(ui_widget_t t)
{
	ui_fg(t.color);
	char *s = t.data;
	// printf("Drawing text %s at %d, %d\n", t.data, t.x, t.y);
	XDrawString(g_d, g_w, g_gc, t.x, t.y + 20, s, strlen(s));
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

void ui_bldbtn(ui_widget_t b)
{
	b.color = RGB(50, 154, 229);
	ui_bldrect(b);
	b.x += 8;
	b.color = RGB(255, 255, 255);
	ui_bldtext(b);
}

ui_widget_t ui_btn(char *text)
{
	ui_widget_t t = ui_text(text);
	t.w += 16;
	t.bld = ui_bldbtn;
	return t;
}

void ui_bldnothing(ui_widget_t w)
{
	// do nothing!
	printf("The buffer is %.*s\n", g_buflen, g_buf);
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

static ui_inputstr_data_t ui_inputstr_data()
{
	return (ui_inputstr_data_t){
		.len = UI_MAX_INPUTSTR_LEN,
		.cursor = 0,
		.focused = 0,
	};
}

void ui_bldinputstr(ui_widget_t i)
{;
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
				memmove(&d->text[d->cursor - 1], &d->text[d->cursor], strlen(d->text) - (d->cursor-1));
				d->cursor--;
			}
		}
		else if (g_keysym == XK_Delete)
		{
			printf("Deleting\n");
			memmove(&d->text[d->cursor], &d->text[d->cursor + 1], strlen(d->text) - d->cursor);
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
	ui_bldrect(i);
	i.x += 8;
	i.color = RGB(0, 0, 0);
	i.data = d->text;
	ui_bldtext(i);
	// draw a little cursor
	if (d->focused)
	{
		ui_widget_t cursorw = ui_rect4(i.x + 6 * d->cursor, i.y + 10, 2, 12);
		cursorw.color = RGB(0, 0, 0);
		cursorw.bld(cursorw);
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
