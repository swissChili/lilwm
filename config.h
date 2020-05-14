#ifndef LILWM_CONFIG_H
#define LILWM_CONFIG_H

#include <fastkv.h>
#include <stdlib.h>

struct config
{
	item_t p;
	char *data;
};

struct config parsefile();
void freeconfig(struct config cfg);
char *kv_strdefault(item_t p, char *k, char *d);
int kv_intdefault(item_t p, char *k, int d);
void exec_autorun(struct config cfg);
void runcmd(char *command);

#endif
