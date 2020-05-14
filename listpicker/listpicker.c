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
	static int j = 0;
	static int real_selected = 0;
	static ui_inputstr_data_t text = UI_INPUTSTR_DATA();

	//printf("Currently selected %d out of %d\n", real_selected, numoptions);

	if (ui_keypressed(w, "Return"))
	{
		//printf("returned and selected %d out of %d\n", real_selected, numoptions);
		printf("%d\n", real_selected);
		exit(0);
	}
	else if (ui_keypressed(w, "Down"))
	{
		if (selected < j)
			selected++;
		w->should_update = true;
	}
	else if (ui_keypressed(w, "Up"))
	{
		if (selected > 0)
			selected--;
		w->should_update = true;
	}

	ui_row(w);
	ui_add(w, ui_inputstr(&text, -1));
	ui_pack(w);

	int inputlen = strlen(text.text);

	j = 0;
	real_selected = 0;
	for (int i = 0; i < numoptions; i++)
	{
		int dist = levenshtein(options[i], text.text);
		int itemlen = strlen(options[i]);
		//printf("levenshtein %s %s = %d\n", options[i], text.text, dist);
		if ((inputlen && itemlen >= inputlen &&
			 dist < MIN(itemlen - inputlen + 1, 10)) ||
			!inputlen)
		{
			int color = UI_BG;
			if (j == selected)
			{
				color = UI_PRIMARY;
				real_selected = i;
				//printf("this is selected: %d\n", real_selected);
			}

			ui_row(w);
			ui_widget_t btn_w = ui_btnc(options[i], color);
			btn_w.w = -1;
			int btn = ui_add(w, btn_w);
			ui_pack(w);
			if (ui_widgetclicked(w, btn))
			{
				if (selected == j)
				{
					printf("%d\n", i);
					exit(0);
				}
				selected = j;
				real_selected = i;
				w->should_update = true;
			}
			j++;
		}
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
