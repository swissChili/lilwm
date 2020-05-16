#include "lilui.h"
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <fastkv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NEW(x) (calloc(sizeof(x), 1))
#define BUF_MAX_LEN 32

static ui_window_t g_windows[UI_MAX_WINDOWS];
static int g_numwindows;

UI_DEF_LIST(ui_widget_t)
UI_DEF_LIST(ui_row_t)

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
	for (int i = 0; i < win->rows.len; i++)
	{
		ui_row_t *current = UI_NTH(win->rows, ui_row_t, i);
		UI_CLEAR(current, ui_widget_t);
	}
	UI_CLEAR(&win->rows, ui_row_t);
}

void ui_setwindow(ui_window_t *win)
{
	XSetICFocus(win->ic);
	XMapWindow(win->dpy, win->win);
}

ui_window_t ui_window(char *name, int width, int height, ui_theme_t theme)
{
	ui_window_t win;
	Display *dpy = XOpenDisplay(NULL);
	int scr = DefaultScreen(dpy);
	XSetWindowAttributes attrs;
	attrs.cursor = XCreateFontCursor(dpy, XC_arrow);
	attrs.background_pixel = WhitePixel(dpy, scr);

	XWindowAttributes root_attrs;
	Window root = RootWindow(dpy, scr);
	XGetWindowAttributes(dpy, root, &root_attrs);
	int x = (root_attrs.width - width) / 2,
		y = (root_attrs.height - height) / 2;

	Window w = XCreateWindow(dpy, root, x, y, width, height, 1, CopyFromParent,
							 InputOutput, CopyFromParent,
							 CWCursor | CWBackPixel, &attrs);

	XStoreName(dpy, w, name);

	if (dpy == NULL)
	{
		fprintf(stderr, "Could not open X display\n");
		exit(EXIT_FAILURE);
	}
	XSelectInput(dpy, w, ExposureMask | KeyPressMask | ButtonPressMask);
	win.dpy = dpy;
	win.scr = scr;
	win.win = w;
	win.rows = UI_NEW_LIST(ui_row_t);
	win.buflen = 0;
	win.theme = theme;
	win.dont_clear = false;

	// xft stuff
	win.draw = XftDrawCreate(dpy, win.win, DefaultVisual(dpy, scr),
							 DefaultColormap(dpy, scr));

	win.font = XftFontOpen(dpy, scr, XFT_FAMILY, XftTypeString, theme.font,
						   XFT_SIZE, XftTypeDouble, theme.font_size, NULL);

	win.im = XOpenIM(dpy, NULL, NULL, NULL);
	win.ic =
		XCreateIC(win.im, XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
				  XNClientWindow, w, NULL);

	g_windows[g_numwindows++] = win;

	return win;
}

void ui_fg(ui_window_t *win, unsigned long color)
{
	XSetForeground(win->dpy, win->gc, color);
}

void ui_fg3(ui_window_t *win, unsigned char r, unsigned char g, unsigned char b)
{
	XSetForeground(win->dpy, win->gc, RGB(r, g, b));
}

ui_row_t *ui_row(ui_window_t *win)
{
	// add an empty row
	UI_ADD(&win->rows, ui_row_t, UI_NEW_LIST(ui_widget_t));
	return &win->rows.last->val;
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

int ui_widgetclicked(ui_window_t *win, ui_widget_t *w)
{
	return ui_isclicked(win, *w);
}

int ui_keypressed(ui_window_t *win, char *key)
{
	return win->keysym == XStringToKeysym(key);
}

int ui_isclicked(ui_window_t *win, ui_widget_t w)
{
	int l = w.x, r = w.x + w.w, t = w.y, b = w.y + w.h;

	if (win->evt.type == UI_EVT_CLICK)
	{
		return win->evt.x >= l && win->evt.x <= r && win->evt.y >= t &&
			   win->evt.y <= b;
	}

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

	for (int r = 0; r < win->rows.len; r++)
	{
		int w = a.width, h = a.height,
			w_w = 0,		// combined width of all widgets
			w_h = 0,		// combined height of all widgets
			flex_num_w = 0, // proportional flexible space
			flex_num_h = 0, max_h = 0;

		ui_row_t *row = UI_NTH(win->rows, ui_row_t, r);

		for (int i = 0; i < row->len; i++)
		{
			ui_widget_t *wdgt = UI_NTH(*row, ui_widget_t, i);
			if (wdgt->w >= 0)
				w_w += wdgt->w;
			else
				flex_num_w += 0 - wdgt->w;

			if (wdgt->h >= 0)
				w_h += wdgt->h;
			else
				flex_num_h += 0 - wdgt->h;
		}
		// flexible space
		int flex_w = flex_num_w ? (w - w_w) / flex_num_w : 0,
			flex_h = flex_num_h ? (h - w_h) / flex_num_h : 0;

		for (int i = 0; i < row->len; i++)
		{
			ui_widget_t *wdgt = UI_NTH(*row, ui_widget_t, i);
			*wdgt = ui_dynsz(win, *wdgt, flex_w, flex_h, &max_h);
			wdgt->bld(win, *wdgt);
		}

		win->x = 0;
		win->y += max_h;
	}
	// //printf("Packed, y is now %d\n", g_y);
}

ui_widget_t *ui_add(ui_row_t *r, ui_widget_t w)
{
	UI_ADD(r, ui_widget_t, w);
	return &r->last->val;
}

void ui_clear(ui_window_t *win, unsigned long color)
{
	ui_fg(win, color);
	XClearWindow(win->dpy, win->win);
}

void ui_clearwinstate(ui_window_t *win)
{
	win->buflen = 0;
	win->keysym = 0;
}

void ui_redraw(ui_window_t *win, ui_rendererloop_t rl)
{
	do
	{
		// Used in WM
		if (!win->dont_clear)
			ui_clear(win, RGB(255, 255, 255));
		win->should_update = false;
		ui_start(win);
		rl(win);
		win->evt.type = UI_EVT_NONE;
		ui_clearwinstate(win);
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
			// printf("Buffer overflow");
		}
		if (status == XLookupKeySym || status == XLookupBoth)
		{
			// printf("Status: %d\n", status);
		}
		// printf("Pressed key %s (%lu)\n", keystr, win->keysym);

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
			// printf("Mouse clicked %d\n", e.xbutton.button);
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

XGlyphInfo ui_glyphinfo(ui_window_t *win, char *text, int len)
{
	XGlyphInfo ext;
	XftTextExtents8(win->dpy, win->font, text, len, &ext);
	return ext;
}

void ui_basictheme(ui_theme_t *t)
{
	char *lilui_theme = getenv("LILUI_THEME");
	if (lilui_theme)
	{
		ui_parsetheme(lilui_theme, t);
	}
	else
	{
		t->font = "Helvetica";
		t->font_size = 12.0;
		t->c[UI_BG] = RGB(255, 255, 255);
		t->c[UI_FG] = RGB(0, 0, 0);
		t->c[UI_PRIMARY] = RGB(89, 127, 249);
		t->c[UI_PRIMARY_ACCENT] = RGB(75, 112, 234);
		t->c[UI_LIGHT] = RGB(211, 214, 226);
		t->c[UI_LIGHT_ACCENT] = RGB(188, 194, 214);
		t->c[UI_DARK] = RGB(19, 20, 25);
		t->c[UI_DARK_ACCENT] = RGB(26, 28, 35);
	}
}

unsigned long ui_kv2rgb(item_t v)
{
	if (v.type != TYPE_STRING)
		return 0;

	unsigned short r, g, b;

	if (sscanf(v.string, "%hd %hd %hd", &r, &g, &b) != 3)
		return 0;

	return RGB(r, g, b);
}

double ui_kv2double(item_t v)
{
	if (v.type != TYPE_STRING)
		return 0;

	return strtod(v.string, NULL);
}

#define CLR(name, val)                                                         \
	if (strcmp(k, #name) == 0)                                                 \
	{                                                                          \
		printf(#name " is %lu\n", ui_kv2rgb(v));                                \
		theme->c[val] = ui_kv2rgb(v);                                          \
	}

void ui_parsetheme(const char *file, ui_theme_t *theme)
{
	FILE *in = fopen(file, "r");
	if (in == NULL)
		return;
	fseek(in, 0, SEEK_END);
	uint64_t size = ftell(in);
	fseek(in, 0, SEEK_SET);

	char *text = malloc(size + 1);
	(void)fread(text, 1, size, in);
	text[size] = 0;

	fclose(in);

	uint64_t i = 0;

	vars_t defines = {
		.length = 0,
		.vars = (char *[]){NULL},
	};

	item_t t = kv_parse(text, &i, size, defines);

	if (t.type != TYPE_OBJECT)
		return;

	theme->theme_source = UI_THEME_ENV;

	for (int i = 0; i < t.length; i++)
	{
		char *k = t.object[i].key.string;
		item_t v = t.object[i].value;

		if (strcmp(k, "font") == 0 && v.type == TYPE_OBJECT)
		{
			item_t family = kv_get(v, "family");
			item_t size = kv_get(v, "size");
			if (family.type == TYPE_STRING)
			{
				theme->font = strdup(family.string);
			}
			if (size.type == TYPE_STRING)
			{
				theme->font_size = ui_kv2double(size);
			}

			printf("font %s @ %f\n", theme->font, theme->font_size);
		}
		else
			CLR(background, UI_BG)
		else CLR(foreground, UI_FG)
		else CLR(primary, UI_PRIMARY)
		else CLR(primary_accent, UI_PRIMARY_ACCENT)
		else CLR(dark, UI_DARK)
		else CLR(dark_accent, UI_DARK_ACCENT)
		else CLR(light, UI_LIGHT)
		else CLR(light_accent, UI_LIGHT_ACCENT)
	}
}

#undef CLR

void ui_freetheme(ui_theme_t *theme)
{
	if (theme->theme_source == UI_THEME_ENV)
	{
		free(theme->font);
	}
}
