#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct config parsefile(char *file)
{
	FILE *in = fopen(file, "r");
	fseek(in, 0, SEEK_END);
	uint64_t size = ftell(in);
	fseek(in, 0, SEEK_SET);

	char *text = malloc(size + 1);
	(void)fread(text, 1, size, in);
	text[size] = 0;

	fclose(in);

	uint64_t i = 0;

	vars_t defines = {
		.length = 0,
		.vars = (char *[]){NULL},
	};

	item_t parsed = kv_parse(text, &i, size, defines);

	return (struct config){
		.p = parsed,
		.data = text,
	};
}

void freeconfig(struct config c)
{
	kv_freeitem(c.p);
	free(c.data);
}

char *kv_strdefault(item_t p, char *k, char *d)
{
	item_t r = kv_query(p, k);
	if (r.type == TYPE_STRING)
		return r.string;
	else
		return d;
}

void runcmd(char *cmd)
{
	// fork time!1
	int i = fork();
	if (i != 0)
	{
		// error or parent
		return;
	}
	exit(system(cmd));
}

void exec_autorun(struct config cfg)
{
	item_t ar = kv_get(cfg.p, "autorun");
	if (ar.type == TYPE_OBJECT)
	{
		for (int i = 0; i < ar.length; i++)
		{
			if (strcmp(ar.object[i].key.string, "cmd") == 0 &&
				ar.object[i].value.type == TYPE_STRING)
			{
				runcmd(ar.object[i].value.string);
			}
		}
	}
}
