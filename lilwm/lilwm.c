#include "config.h"
#include "keys.h"
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define XKEY(dpy, k) XKeysymToKeycode(dpy, XStringToKeysym(k))

#define MOD Mod1Mask

int kv_xkey(Display *dpy, item_t i, char *q, char *undef)
{
	return XKEY(dpy, kv_strdefault(i, q, undef));
}

int main(int argc, char **argv)
{
	// return value
	int result = 0;

	struct config cfg;
	if (argc >= 2)
	{
		printf("Loading %s\n", argv[1]);
		cfg = parsefile(argv[1]);
	}
	else
	{
		puts("Loading /etc/lilwm/lilwmrc");
		cfg = parsefile("/etc/lilwm/lilwmrc");
	}

	Display *dpy;
	Window root;
	XWindowAttributes attr;
	XButtonEvent start;
	XEvent ev;

	if (!(dpy = XOpenDisplay(0x0)))
		return 1;

	int s = DefaultScreen(dpy);

#ifdef NDEBUG
	root = DefaultRootWindow(dpy);
#else
	root = XCreateSimpleWindow(dpy, RootWindow(dpy, s), 10, 10, 640, 480, 1,
							   BlackPixel(dpy, s), WhitePixel(dpy, s));
#endif

	char *termcmd = kv_strdefault(cfg.p, "apps.terminal", "xterm");

	item_t keys = kv_query(cfg.p, "keys");

	int tile_gap = kv_intdefault(cfg.p, "tile.gap", 16);

	if (keys.type != TYPE_OBJECT)
	{
		// defaults
		fprintf(stderr, "The keys fieled in your lilwmrc is empty");
		result = EXIT_FAILURE;
		goto finish;
	}
	else
	{
		for (int i = 0; i < keys.length; i++)
		{
			pair_t c = keys.object[i];
			char *k = c.key.string;
			wm_key_t key = wm_str2key(k);
			printf("Grabbing key %s\n", k);
			XGrabKey(dpy, XKeysymToKeycode(dpy, key.keysym), key.mask, root,
					 True, GrabModeAsync, GrabModeAsync);
		}
	}

	XGrabButton(dpy, 1, MOD, root, True,
				ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
				GrabModeAsync, GrabModeAsync, None, None);
	XGrabButton(dpy, 3, MOD, root, True,
				ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
				GrabModeAsync, GrabModeAsync, None, None);

#ifndef NDEBUG
	XMapWindow(dpy, root);
#endif
	XSetWindowAttributes attrs;
	attrs.cursor = XCreateFontCursor(dpy, XC_arrow);
	XChangeWindowAttributes(dpy, root, CWCursor, &attrs);

	// open a terminal
	runcmd(kv_strdefault(cfg.p, "apps.terminal", "xterm"));
	// run autorun script
	exec_autorun(cfg);

	// XSetInputFocus(dpy, root, RevertToParent, CurrentTime);

	for (;;)
	{
		XNextEvent(dpy, &ev);

		if (ev.type == KeyPress)
		{
			KeySym ks = XLookupKeysym(&ev.xkey, 0);
			wm_key_t k;
			k.keysym = ks;
			k.mask = ev.xkey.state;

			for (int i = 0; i < keys.length; i++)
			{
				wm_key_t itemkey = wm_str2key(keys.object[i].key.string);
				item_t cmd = keys.object[i].value;
				if (cmd.type == TYPE_STRING && wm_keyeq(itemkey, k))
				{
					// run this command
					if (strcmp(cmd.string, "focus") == 0 &&
						ev.xkey.subwindow != None)
					{
						XRaiseWindow(dpy, ev.xkey.subwindow);
					}
					else if (strcmp(cmd.string, "newterm") == 0)
					{
						runcmd(termcmd);
					}
					else if (strcmp(cmd.string, "closewin") == 0 &&
							 ev.xkey.subwindow != None)
					{
						XDestroyWindow(dpy, ev.xkey.subwindow);
					}
					else if (strcmp(cmd.string, "quit") == 0)
					{
						int quit = system(
							"msgbox 'Are you sure you want to quit lilwm?'");
						if (!quit)
							goto finish;
					}
					else if (strncmp(cmd.string, "tile", 4) == 0 &&
							 ev.xkey.subwindow != None)
					{
						char side[12];
						if (sscanf(cmd.string, "tile %s", side))
						{
							XWindowAttributes attrs;
							XGetWindowAttributes(dpy, root, &attrs);
							if (strcmp(side, "right") == 0)
							{
								XMoveResizeWindow(
									dpy, ev.xkey.subwindow,
									(attrs.width / 2) + tile_gap / 2, tile_gap,
									(attrs.width / 2) - 1.5 * tile_gap,
									attrs.height - 2 * tile_gap);
							}
							else if (strcmp(side, "left") == 0)
							{
								XMoveResizeWindow(
									dpy, ev.xkey.subwindow, tile_gap, tile_gap,
									(attrs.width / 2) - 1.5 * tile_gap,
									attrs.height - 2 * tile_gap);
							}
						}
					}
					else if (strncmp(cmd.string, "shell", 5) == 0)
					{
						runcmd(cmd.string + 5);
					}
				}
			}
		}
		else if (ev.type == ButtonPress && ev.xbutton.subwindow != None)
		{
			XGrabPointer(dpy, ev.xbutton.subwindow, True,
						 PointerMotionMask | ButtonReleaseMask, GrabModeAsync,
						 GrabModeAsync, None, None, CurrentTime);
			XRaiseWindow(dpy, ev.xbutton.subwindow);
			// XSetInputFocus(dpy, ev.xbutton.subwindow, RevertToParent,
			// CurrentTime);

			XGetWindowAttributes(dpy, ev.xbutton.subwindow, &attr);
			start = ev.xbutton;
		}
		else if (ev.type == MotionNotify)
		{
			int xdiff, ydiff;
			while (XCheckTypedEvent(dpy, MotionNotify, &ev))
				;
			xdiff = ev.xbutton.x_root - start.x_root;
			ydiff = ev.xbutton.y_root - start.y_root;
			XMoveResizeWindow(
				dpy, ev.xmotion.window,
				attr.x + (start.button == 1 ? xdiff : 0),
				attr.y + (start.button == 1 ? ydiff : 0),
				MAX(1, attr.width + (start.button == 3 ? xdiff : 0)),
				MAX(1, attr.height + (start.button == 3 ? ydiff : 0)));
		}
		else if (ev.type == ButtonRelease)
		{
			start.subwindow = None;
			XUngrabPointer(dpy, CurrentTime);
		}
	}

finish:
	// nuke everything
	XDestroySubwindows(dpy, root);
	if (dpy)
		XCloseDisplay(dpy);

	freeconfig(cfg);
	return result;
}
