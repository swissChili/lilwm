#pragma once

#include <X11/Xlib.h>

#define RGB(r, g, b) (b + (g<<8) + (r<<16))
#define MAX(a, b) (a > b ? a : b)

// maximum number of widgets per row
#define UI_MAX_PER_ROW 32

typedef struct ui_widget_t
{
	/**
	 * x and y are the coordinates of the top-left corner of the widget.
	 * Default -1 for auto-layout (recommended).
	 */
	int x, y;
	/**
	 * w and h are the width and height of the widget. 0 for minimum
	 * viable size. Negative for dynamic sizing based on remaining space.
	 */
	int w, h;
	/**
	 * Widget data
	 */
	void *data;
	/**
	 * Function pointer to build (draw) this widget.
	 */
	void (*bld)(struct ui_widget_t);
	// color
	unsigned long color;
	/**
	 * Destructor
	 */
	void (*del)(struct ui_widget_t);
} ui_widget_t;

typedef struct ui_row_t
{
	unsigned len;
	ui_widget_t wdgts[UI_MAX_PER_ROW];
} ui_row_t;

typedef struct ui_window_t
{
	Window win;
	Display *dpy;
	int scr;
} ui_window_t;

typedef struct ui_mouseevent_t
{
	enum
	{
		UI_EVT_NONE = 0,
		UI_EVT_CLICK = 1,
		UI_EVT_RIGHTCLICK = 3,
	} type;
	int x, y;
} ui_mouseevent_t;

typedef struct ui_ctx_t
{
	int should_update;
} ui_ctx_t;

typedef void (* ui_rendererloop_t)(ui_ctx_t *);

ui_window_t ui_window();
void ui_setwindow(ui_window_t win);
void ui_loop(ui_rendererloop_t rl);
void ui_init();
void ui_start();
void ui_row();
void ui_pack();
int ui_isclicked(ui_widget_t w);
int ui_widgetclicked(int i);
int ui_add(ui_widget_t w);
void ui_clear(unsigned long color);
// widgets
ui_widget_t ui_rectc(int w, int h, long color);
ui_widget_t ui_rect(int w, int h);
ui_widget_t ui_text(char *text);
ui_widget_t ui_btn(char *text);
ui_widget_t ui_hspacer(int size);
ui_widget_t ui_vspacer(int size);
