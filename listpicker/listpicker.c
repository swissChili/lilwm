#include "levenshtein.h"
#include <X11/Xlib.h>
#include <lilui.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char **options;
static int numoptions;

void draw(ui_window_t *w)
{
	static int selected = 0;
	static ui_inputstr_data_t text = UI_INPUTSTR_DATA();

	ui_row(w);
	ui_add(w, ui_inputstr(&text, -1));
	ui_pack(w);

	int inputlen = strlen(text.text);

	for (int i = 0; i < numoptions; i++)
	{
		int dist = levenshtein(options[i], text.text);
		printf("levenshtein %s %s = %d\n", options[i], text.text, dist);
		if ((inputlen && dist < strlen(options[i]) - inputlen + 2) || !inputlen)
		{
			int color = UI_BG;
			if (i == selected)
			{
				color = UI_PRIMARY;
			}

			ui_row(w);
			ui_widget_t btn_w = ui_btnc(options[i], color);
			btn_w.w = -1;
			int btn = ui_add(w, btn_w);
			ui_pack(w);
			if (ui_widgetclicked(w, btn))
			{
				if (selected == i)
				{
					printf("%d\n", i);
					exit(0);
				}
				selected = i;
				w->should_update = true;
			}
		}
	}

	printf("Currently selected %d out of %d\n", selected, numoptions);

	if (ui_keypressed(w, "Return"))
	{
		printf("%d\n", selected);
		exit(0);
	}
	else if (ui_keypressed(w, "Down"))
	{
		if (selected < numoptions - 1)
			selected++;
		w->should_update = true;
	}
	else if (ui_keypressed(w, "Up"))
	{
		if (selected > 0)
			selected--;
		w->should_update = true;
	}
}

int main(int argc, char **argv)
{
	if (argc <= 1)
		return 0;

	options = argv + 1;
	numoptions = argc - 1;

	ui_theme_t theme;
	ui_basictheme(&theme);
	ui_window_t win = ui_window(240, 240, theme);
	ui_setwindow(&win);
	ui_loop(draw);
}
