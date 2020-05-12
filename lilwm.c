#include <X11/Xlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

void run_cmd(char *cmd)
{
	// fork time!1
	int i = fork();
	if (i != 0)
	{
		// error or parent
		return;
	}
	system(cmd);
}

#define XKEY(dpy, k) XKeysymToKeycode(dpy, XStringToKeysym(k))

int kv_xkey(Display *dpy, item_t i, char *q, char *undef)
{
	return XKEY(dpy, kv_strdefault(i,q, undef));
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
			printf("Grabbing key %s\n", k);
			XGrabKey(dpy, XKEY(dpy, k), Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);
		}
	}

	XGrabButton(dpy, 1, Mod1Mask, root, True,
				ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
	XGrabButton(dpy, 3, Mod1Mask, root, True,
				ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);

#ifndef NDEBUG
	XMapWindow(dpy, root);
#endif

	for (;;)
	{
		XNextEvent(dpy, &ev);
		if (ev.type == KeyPress)
		{
			char *k = XKeysymToString(XLookupKeysym(&ev.xkey, 0));
			printf("Key pressed %s\n", k);

			item_t cmd = kv_get(keys, k);
			kv_printitem(cmd, 1);
			if (cmd.type == TYPE_STRING)
			{
				// run this command
				if (strcmp(cmd.string, "focus") == 0 && ev.xkey.subwindow != None)
				{
					XRaiseWindow(dpy, ev.xkey.subwindow);
				}
				else if (strcmp(cmd.string, "newterm") == 0)
				{
					run_cmd(termcmd);
				}
				else if (strcmp(cmd.string, "closewin") == 0 && ev.xkey.subwindow != None)
				{
					XDestroyWindow(dpy, ev.xkey.subwindow);
				}
				else if (strcmp(cmd.string, "quit") == 0)
				{
					goto finish;
				}
			}
		}
		else if (ev.type == ButtonPress && ev.xbutton.subwindow != None)
		{
			XGrabPointer(dpy, ev.xbutton.subwindow, True,
						 PointerMotionMask | ButtonReleaseMask, GrabModeAsync,
						 GrabModeAsync, None, None, CurrentTime);
			XRaiseWindow(dpy, ev.xbutton.subwindow);
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
	if (dpy)
		XCloseDisplay(dpy);

	freeconfig(cfg);
	return result;
}
