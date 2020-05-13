#include <X11/Xlib.h>
#include <stdio.h>
#include <lilui.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static char *msg;

void draw(ui_ctx_t *ctx)
{
	ui_row();
	ui_add(ui_hspacer(-1));
	ui_add(ui_text(msg));
	ui_add(ui_hspacer(-1));
	ui_pack();

	ui_row();
	ui_add(ui_hspacer(-1));
	int cancel = ui_add(ui_btn("Cancel"));
	ui_add(ui_hspacer(-1));
	int ok = ui_add(ui_btn("Ok"));
	ui_add(ui_hspacer(-1));
	ui_pack();

	if (ui_widgetclicked(cancel))
	{
		exit(1);
	}
	if (ui_widgetclicked(ok))
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
	ui_window_t win = ui_window(MAX(strlen(msg) * 6 + 24, 120), 75);
	ui_setwindow(&win);
	ui_loop(draw);
}
