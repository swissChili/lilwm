#include <X11/Xlib.h>
#include <lilui.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *msg;

void draw(ui_window_t *w)
{
	ui_row_t *msgrow = ui_row(w);
	ui_add(msgrow, ui_hspacer(-1));
	ui_add(msgrow, ui_text(w, msg));
	ui_add(msgrow, ui_hspacer(-1));
	ui_add(msgrow, ui_vspacer(-1));

	ui_row_t *btnrow = ui_row(w);
	ui_add(btnrow, ui_hspacer(-1));
	ui_widget_t *cancel = ui_add(btnrow, ui_btn(w, "Cancel"));
	ui_add(btnrow, ui_hspacer(10));
	ui_widget_t *ok = ui_add(btnrow, ui_btn(w, "Ok"));
	ui_add(btnrow, ui_hspacer(-1));

	ui_pack(w);

	if (ui_widgetclicked(w, cancel))
	{
		exit(1);
	}
	if (ui_widgetclicked(w, ok))
	{
		exit(0);
	}
}

int main(int argc, char **argv)
{
	if (argc >= 2)
	{
		msg = argv[1];
	}
	else
	{
		msg = "Provide your message as the first argument to msgbox.";
	}
	ui_theme_t theme;
	ui_basictheme(&theme);
	ui_window_t win = ui_window("Message Box", 360, 120, theme);
	ui_setwindow(&win);
	ui_loop(draw);
}
