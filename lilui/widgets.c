#include "lilui.h"
#include <X11/Xlib.h>
#include <X11/keysym.h>

void ui_bldrect(ui_window_t *win, ui_widget_t w)
{
	ui_fg(win, w.color);
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
	ui_fg(win, t.color);
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
	printf("keysym: %d, Left: %d\n", win->keysym, XK_Left);
	if (ui_isclicked(win, i))
	{
		printf("Focused ui_inputstr\n");
		d->focused = 1;
	}
	if (ui_clickedoff(win, i))
	{
		printf("Clicked off ui_inputstr\n");
		d->focused = 0;
	}
	if (d->focused)
	{
		if (win->keysym == XK_BackSpace)
		{
			if (strlen(d->text) > 0)
			{
				memmove(&d->text[d->cursor - 1], &d->text[d->cursor],
						strlen(d->text) - (d->cursor - 1));
				d->cursor--;
			}
		}
		else if (win->keysym == XK_Delete)
		{
			printf("Deleting\n");
			memmove(&d->text[d->cursor], &d->text[d->cursor + 1],
					strlen(d->text) - d->cursor);
		}
		else if (win->keysym == XK_Left)
		{
			printf("Cursor Left %d\n", d->cursor);
			if (d->cursor > 0)
				d->cursor--;
		}
		else if (win->keysym == XK_Right)
		{
			if (d->cursor < strlen(d->text))
				d->cursor++;
		}
		else if (win->keysym == XK_Up)
		{
			d->cursor = 0;
		}
		else if (win->keysym == XK_Down)
		{
			d->cursor = strlen(d->text);
		}
		else if (win->buflen && d->len > strlen(d->text) + win->buflen)
		{
			printf("appending to buffer %.*s\n", win->buflen, win->buf);
			win->buf[win->buflen] = 0;
			printf("buf is %s, %s\n", win->buf, d->text);
			strncat(d->text, win->buf, win->buflen);
			d->cursor += win->buflen;
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

void ui_bldprogressbar(ui_window_t *win, ui_widget_t b)
{
	b.color = RGB(42, 48, 48);
	ui_bldrect(win, b);

	double progress = *(double *)b.data;
	int width = MIN(b.w, b.w * progress);
	printf("progress bar at %f (%d)\n", progress, width);
	b.w = width;
	b.color = RGB(9, 145, 150);
	ui_bldrect(win, b);
}

ui_widget_t ui_progressbar(double *progress, int wlen)
{
	return (ui_widget_t){
		.x = -1,
		.y = -1,
		.w = wlen,
		.h = 32,
		.bld = ui_bldprogressbar,
		.data = progress,
	};
}
