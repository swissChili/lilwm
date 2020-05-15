#include "lilui.h"
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <string.h>

void ui_bldrect(ui_window_t *win, ui_widget_t w)
{
	ui_fg(win, win->theme.c[w.theme_color]);
	XFillRectangle(win->dpy, win->win, win->gc, w.x, w.y, w.w, w.h);
}

ui_widget_t ui_rectc(int w, int h, int c)
{
	return (ui_widget_t){
		.x = -1,
		.y = -1,
		.w = w,
		.h = h,
		.theme_color = c,
		.bld = ui_bldrect,
	};
}

ui_widget_t ui_rect(int w, int h)
{
	return ui_rectc(w, h, UI_DARK);
}

ui_widget_t ui_rect4(int x, int y, int w, int h)
{
	return (ui_widget_t){
		.x = x,
		.y = y,
		.w = w,
		.h = h,
		.theme_color = UI_DARK,
		.bld = ui_bldrect,
	};
}

#define TEXT_MARGIN 12

void ui_bldtext(ui_window_t *win, ui_widget_t t)
{
	// ui_fg(win, win->theme.c[t.theme_color]);
	char *s = t.data;

	XftColor xftcolor;
	xftcolor.color.alpha = 0xFFFF;
	xftcolor.color.red = 0;
	xftcolor.color.green = 0;
	xftcolor.color.blue = 0;

	XftDrawStringUtf8(win->draw, &xftcolor, win->font, t.x + TEXT_MARGIN,
					  t.y + (t.h - TEXT_MARGIN), s, strlen(s));
}

ui_widget_t ui_text(ui_window_t *win, char *text)
{
	XGlyphInfo ext = ui_glyphinfo(win, text, strlen(text));

	return (ui_widget_t){
		.x = -1,
		.y = -1,
		.w = ext.width + TEXT_MARGIN * 2,
		.h = ext.height + TEXT_MARGIN * 2,
		.theme_color = UI_FG,
		.data = (void *)text,
		.bld = ui_bldtext,
	};
}

void ui_bldbtn(ui_window_t *win, ui_widget_t b)
{
	ui_bldrect(win, b);
	b.x += 8;
	b.theme_color = UI_FG;
	ui_bldtext(win, b);
}

ui_widget_t ui_btnc(ui_window_t *win, char *text, int c)
{
	ui_widget_t t = ui_text(win, text);
	t.w += 16;
	t.theme_color = c;
	t.bld = ui_bldbtn;
	return t;
}

ui_widget_t ui_btn(ui_window_t *win, char *text)
{
	return ui_btnc(win, text, UI_PRIMARY);
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
	// printf("Currnet focus is %d\n", d->focused);
	// printf("keysym: %d, Left: %d\n", win->keysym, XK_Left);
	if (ui_isclicked(win, i))
	{
		// printf("Focused ui_inputstr\n");
		d->focused = 1;
	}
	if (ui_clickedoff(win, i))
	{
		// printf("Clicked off ui_inputstr\n");
		d->focused = 0;
	}
	if (d->focused)
	{
		// stupid, horrible hack to make the rest of the widgets update
		// correctly.
		if (win->keysym)
			win->should_update = true;

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
			// printf("Deleting\n");
			memmove(&d->text[d->cursor], &d->text[d->cursor + 1],
					strlen(d->text) - d->cursor);
		}
		else if (win->keysym == XK_Left)
		{
			// printf("Cursor Left %d\n", d->cursor);
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
			// printf("appending to buffer %.*s\n", win->buflen, win->buf);
			win->buf[win->buflen] = 0;
			// printf("buf is %s, %s\n", win->buf, d->text);
			strncat(d->text, win->buf, win->buflen);
			d->cursor += win->buflen;
		}
	}

	// puts("setting rect color");
	i.theme_color = UI_LIGHT_ACCENT;
	ui_bldrect(win, i);
	i.x += 8;
	i.theme_color = UI_FG;
	i.data = d->text;
	ui_bldtext(win, i);
	// draw a little cursor
	if (d->focused)
	{
		XGlyphInfo info = ui_glyphinfo(win, d->text, d->cursor);
		int cursorx = info.width + 21;
		ui_widget_t cursorw = ui_rect4(cursorx, i.y + 6, 2, 16);
		cursorw.theme_color = RGB(0, 0, 0);
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
	b.theme_color = UI_DARK;
	ui_bldrect(win, b);

	double progress = *(double *)b.data;
	int width = MIN(b.w, b.w * progress);
	// printf("progress bar at %f (%d)\n", progress, width);
	b.w = width;
	b.theme_color = UI_PRIMARY;
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
