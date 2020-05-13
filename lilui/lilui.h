#pragma once

#include <X11/Xlib.h>
#include <stdbool.h>

#define RGB(r, g, b) (b + (g << 8) + (r << 16))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) > (b) ? (b) : (a))
#ifndef UI_MAX_INPUTSTR_LEN
#define UI_MAX_INPUTSTR_LEN 128
#endif

// maximum number of widgets per row
#define UI_MAX_PER_ROW 32

// maximum number of windows
#define UI_MAX_WINDOWS 12

// maximum number of qued X11 keystrokes
#define UI_MAX_BUF_LEN 8

typedef struct ui_mouseevent_t
{
	enum
	{
		UI_MOUSE_DOWN,
		UI_MOUSE_UP,
	} evt;
	enum
	{
		UI_EVT_NONE = 0,
		UI_EVT_CLICK = 1,
		UI_EVT_RIGHTCLICK = 3,
	} type;
	int x, y;
} ui_mouseevent_t;

typedef struct ui_keyevent_t
{
	enum
	{
		UI_KEY_PRESSED,
	} type;
	unsigned int keysym;
	char *str;
} ui_keyevent_t;

// forward decl
struct ui_widget_t;
struct ui_window_t;

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
	void (*bld)(struct ui_window_t *win, struct ui_widget_t);
	// color
	unsigned long color;
	/**
	 * Destructor
	 */
	void (*del)(void *);
} ui_widget_t;

typedef struct ui_row_t
{
	unsigned len;
	struct ui_widget_t wdgts[UI_MAX_PER_ROW];
} ui_row_t;

typedef struct ui_window_t
{
	enum
	{
		UI_WIN_TOPLEVEL,
		UI_WIN_FLOATING,
		UI_WIN_VIRTUAL,
	} type;
	ui_mouseevent_t evt;
	Window win;
	Display *dpy;
	int scr;
	XIM im;
	XIC ic;
	GC gc;
	ui_row_t row;
	int x, y;
	bool should_update;
	char buf[UI_MAX_BUF_LEN];
	int buflen;
	KeySym keysym;
} ui_window_t;

typedef struct ui_ctx_t
{
	int should_update;
} ui_ctx_t;

typedef struct ui_inputstr_data_t
{
	char text[UI_MAX_INPUTSTR_LEN];
	int len;
	int focused;
	int cursor;
} ui_inputstr_data_t;

#define UI_INPUTSTR_DATA() {.len = UI_MAX_INPUTSTR_LEN}

typedef void (*ui_rendererloop_t)(ui_window_t *);

ui_window_t ui_window(int w, int h);
void ui_setwindow(ui_window_t *win);
void ui_loop(ui_rendererloop_t rl);
void ui_init(ui_window_t *win);
void ui_start(ui_window_t *win);
void ui_row(ui_window_t *win);
void ui_pack(ui_window_t *win);
int ui_isclicked(ui_window_t *win, ui_widget_t w);
int ui_widgetclicked(ui_window_t *win, int i);
int ui_add(ui_window_t *win, ui_widget_t w);
void ui_clear(ui_window_t *win, unsigned long color);
// widgets
ui_widget_t ui_rectc(int w, int h, long color);
ui_widget_t ui_rect4(int x, int y, int w, int h);
ui_widget_t ui_rect(int w, int h);
ui_widget_t ui_text(char *text);
ui_widget_t ui_btn(char *text);
ui_widget_t ui_inputstr(ui_inputstr_data_t *data, int widget_len);
ui_widget_t ui_hspacer(int size);
ui_widget_t ui_vspacer(int size);
