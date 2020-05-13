#include <X11/Xlib.h>
#include <stdio.h>
#include <lilui.h>
#include <stdbool.h>
#include <stdlib.h>

void draw(ui_window_t *w)
{
	static char *msg = "Not clicked";
	static char input[128] = "hi";
	static int focused = 0;
	static int cursor = 2;
	static ui_inputstr_data_t data = UI_INPUTSTR_DATA();

	ui_row(w);
	ui_add(w, ui_inputstr(&data, -1));
	ui_pack(w);

	ui_row(w);
	ui_add(w, ui_hspacer(-1));
	ui_add(w, ui_text(msg));
	ui_add(w, ui_hspacer(-1));
	ui_pack(w);

	ui_row(w);
	ui_add(w, ui_hspacer(-1));
	int txt = ui_add(w, ui_btn("Click me!!"));
	ui_add(w, ui_hspacer(-1));
	ui_pack(w);

	if (ui_widgetclicked(w, txt))
	{
		msg = "Clicked!";
		// fine
		w->should_update = true;
	}
}

int main()
{
	ui_window_t win = ui_window(640, 480);
	ui_setwindow(&win);
	ui_loop(draw);
}
