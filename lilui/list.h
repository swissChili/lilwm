#pragma once

// give me generics please, i hate this

#define UI_DECL_LIST(t)                                                        \
	typedef struct ui_node_##t##_t                                             \
	{                                                                          \
		t val;                                                                 \
		struct ui_node_##t##_t *next;                                          \
	} ui_node_##t##_t;                                                         \
	typedef struct                                                             \
	{                                                                          \
		ui_node_##t##_t *first;                                                \
		ui_node_##t##_t *last;                                                 \
		long len;                                                              \
	} ui_list_##t##_t;                                                         \
	t *ui_nth_##t(ui_list_##t##_t l, long n);                                  \
	void ui_add_##t(ui_list_##t##_t *l, t item);                               \
	void ui_clear_##t(ui_list_##t##_t *l);

// also messes up debugging (sad)
// only fix is to add a bunch of #line directives
#define UI_DEF_LIST(t)                                                         \
	t *ui_nth_##t(ui_list_##t##_t l, long n)                                   \
	{                                                                          \
		if (n >= l.len)                                                        \
			return NULL;                                                       \
		ui_node_##t##_t *c = l.first;                                          \
		for (int i = 0; i < n; i++)                                            \
		{                                                                      \
			c = c->next;                                                       \
		}                                                                      \
		return &c->val;                                                        \
	}                                                                          \
                                                                               \
	void ui_add_##t(ui_list_##t##_t *l, t i)                                   \
	{                                                                          \
		ui_node_##t##_t *c = calloc(sizeof(ui_node_##t##_t), 1);               \
		c->val = i;                                                            \
		c->next = NULL;                                                        \
		if (l->len == 0)                                                       \
		{                                                                      \
			l->first = c;                                                      \
			l->last = c;                                                       \
		}                                                                      \
		else                                                                   \
		{                                                                      \
			l->last->next = c;                                                 \
			l->last = c;                                                       \
		}                                                                      \
		l->len++;                                                              \
	}                                                                          \
                                                                               \
	void ui_clear_##t(ui_list_##t##_t *l)                                      \
	{                                                                          \
		if (!l->len)                                                           \
			return;                                                            \
		for (ui_node_##t##_t *c = l->first; c; c = c->next)                    \
		{                                                                      \
			free(c);                                                           \
		}                                                                      \
		l->len = 0;                                                            \
		l->first = NULL;                                                       \
		l->last = NULL;                                                        \
	}

#define UI_NEW_LIST(t)                                                         \
	(ui_list_##t##_t)                                                          \
	{                                                                          \
		.len = 0, .first = NULL, .last = NULL                                  \
	}
#define UI_LIST(t) ui_list_##t##_t
#define UI_NTH(list, t, n) ui_nth_##t(list, n);
#define UI_ADD(list, t, i) ui_add_##t(list, i);
#define UI_CLEAR(list, t) ui_clear_##t(list);
