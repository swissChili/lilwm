#include <X11/Xlib.h>
#include <lilui.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

void draw(ui_window_t *w)
{
	static char *msg = "Not clicked";
	static char input[128] = "hi";
	static int focused = 0;
	static int cursor = 2;
	static double progress = 0.05;
	static ui_inputstr_data_t data = UI_INPUTSTR_DATA();

	ui_row_t *progrow = ui_row(w);
	ui_add(progrow, ui_inputstr(&data, 200));
	ui_add(progrow, ui_progressbar(&progress, -1));

	ui_row_t *msgrow = ui_row(w);
	ui_add(msgrow, ui_hspacer(-1));
	ui_add(msgrow, ui_text(w, msg));
	ui_add(msgrow, ui_hspacer(-1));

	ui_row_t *btnrow = ui_row(w);
	ui_add(btnrow, ui_hspacer(-1));
	ui_widget_t *txt = ui_add(btnrow, ui_btn(w, "Click me!!"));
	ui_add(btnrow, ui_hspacer(-1));

	ui_pack(w);

	if (ui_widgetclicked(w, txt))
	{
		msg = "Clicked!";
		progress += 0.1;
		// fine
		w->should_update = true;
	}
}

int main()
{
	ui_theme_t theme;
	ui_basictheme(&theme);
	ui_window_t win = ui_window("Test", 640, 480, theme);
	ui_setwindow(&win);
	ui_loop(draw);
}
