#include <X11/Xlib.h>
#include <stdio.h>
#include <lilui.h>
#include <stdbool.h>
#include <stdlib.h>

void draw(ui_ctx_t *ctx)
{
	static char *msg = "Not clicked";
	static char input[128] = "hi";
	static int focused = 0;
	static int cursor = 2;
	
	ui_row();
	ui_add(ui_inputstr((char **)&input, &focused, &cursor, 128, -1));
	ui_pack();

	ui_row();
	ui_add(ui_hspacer(-1));
	ui_add(ui_text(msg));
	ui_add(ui_hspacer(-1));
	ui_pack();

	ui_row();
	ui_add(ui_hspacer(-1));
	int txt = ui_add(ui_btn("Click me!!"));
	ui_add(ui_hspacer(-1));
	ui_pack();

	if (ui_widgetclicked(txt))
	{
		msg = "Clicked!";
		// fine
		ctx->should_update = true;
	}
}

int main()
{
	ui_window_t win = ui_window();
	ui_setwindow(win);
	ui_loop(draw);
}
