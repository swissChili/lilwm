#pragma once

#include <X11/Xlib.h>
#include <stdbool.h>

typedef struct wm_key_t
{
	KeySym keysym;
	unsigned mask;
} wm_key_t;

wm_key_t wm_str2key(char *k);
bool wm_keyeq(wm_key_t a, wm_key_t b);
