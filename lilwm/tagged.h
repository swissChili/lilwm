#pragma once

#include <X11/Xlib.h>
#include "../lilui/list.h"

typedef struct taggedwin_t
{
	Window win;
	int tag;
} taggedwin_t;

UI_DECL_LIST(taggedwin_t)
#define winlist_t UI_LIST(taggedwin_t)

typedef struct winvisitor_t
{
	winlist_t *list;
	int last;
} winvisitor_t;

winvisitor_t visitor(winlist_t l);
taggedwin_t *visit_next_tagged(winvisitor_t *v, int tag);
