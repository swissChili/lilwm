#include <string.h>
#include <stdio.h>
#include "lilui.h"

static Display	*g_d;
static Window	g_w;
static int		g_s;
static GC		g_gc;
static ui_row_t	g_row;

static int		g_x = 0;
static int		g_y = 0;

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
	XMapWindow(g_d, g_w);
}

ui_window_t ui_window()
{
	ui_window_t win;
	Display *dpy = XOpenDisplay(NULL);
	int scr = DefaultScreen(dpy);
	Window w = XCreateSimpleWindow(dpy, RootWindow(dpy, scr), 10, 10, 100, 100, 1,
						BlackPixel(dpy, scr), WhitePixel(dpy, scr));
	if (dpy == NULL)
	{
		fprintf(stderr, "Could not open X display\n");
	}
	XSelectInput(dpy, w, ExposureMask | KeyPressMask | ButtonPressMask);
	win.dpy = dpy;
	win.scr = scr;
	win.win = w;

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
	if (i >= g_row.len) return 0;
	return ui_isclicked(g_row.wdgts[i]);
}

int ui_isclicked(ui_widget_t w)
{
	int l = w.x,
		r = w.x + w.w,
		t = w.y,
		b = w.y + w.h;

	if (g_evt.type) printf("evt %d at %d, %d\n", g_evt.type, g_evt.x, g_evt.y);
	printf("l r t b %d %d %d %d\n", l, r, t, b);

	if (g_evt.type == UI_EVT_CLICK)
	{
		return g_evt.x >= l && g_evt.x <= r &&
				g_evt.y >= t && g_evt.y <= b;
	}
	else return 0;
}

void ui_pack()
{
	XWindowAttributes a;
	XGetWindowAttributes(g_d, g_w, &a);
	int w = a.width,
		h = a.height,
		w_w = 0, // combined width of all widgets
		w_h = 0, // combined height of all widgets
		flex_num_w = 0, // proportional flexible space
		flex_num_h = 0,
		max_h = 0;

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
	//printf("Packed, y is now %d\n", g_y);
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
	ui_ctx_t ctx = { 0 };
	do
	{
		ui_clear(RGB(255, 255, 255));
		ctx.should_update = 0;
		ui_start();
		rl(&ctx);
		g_evt.type = UI_EVT_NONE;
	}
	while (ctx.should_update);
}

void ui_loop(ui_rendererloop_t rl)
{
	ui_init();
	XEvent e;
	while (1)
	{
		g_evt.type = UI_EVT_NONE;

		XNextEvent(g_d, &e);
		if (e.type == Expose)
		{
			ui_redraw(rl);
		}
		if (e.type == ButtonPress)
		{
			printf("Button pressed %d\n", e.xbutton.button);
			int x = e.xbutton.x,
				y = e.xbutton.y;
			g_evt = (ui_mouseevent_t){
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

void ui_bldtext(ui_widget_t t)
{
	ui_fg(t.color);
	//printf("Drawing text %s at %d, %d\n", t.data, t.x, t.y);
	XDrawString(g_d, g_w, g_gc, t.x, t.y + 20, t.data, strlen(t.data));
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
}

ui_widget_t ui_hspacer(int size)
{
	return (ui_widget_t){
		.x = -1,
		.y = -1,
		.w = size,
		.h = 1,
		.bld = ui_bldnothing
	};
}

ui_widget_t ui_vspacer(int size)
{
	return (ui_widget_t){
		.x = -1,
		.y = -1,
		.w = 1,
		.h = size,
		.bld = ui_bldnothing
	};
}
