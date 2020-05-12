#include <X11/Xlib.h>
#include <stdio.h>
#include <lilui.h>
#include <stdlib.h>

void draw()
{
	ui_row();
	ui_add(ui_rect(100, 100));
	ui_add(ui_text("Hello"));
	ui_add(ui_rect(100, 32));
	ui_pack();

	ui_row();
	//ui_add(ui_rect(-1, 32));
	int txt = ui_add(ui_btn("Hmm, yess... this is a large text"));
	ui_pack();

	if (ui_widgetclicked(txt))
	{
		printf("text clicked\n");
	}
}

int main()
{
	ui_window_t win = ui_window();
	ui_setwindow(win);
	ui_loop(draw);
}
