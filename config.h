#ifndef LILWM_CONFIG_H
#define LILWM_CONFIG_H

#include <stdlib.h>
#include <fastkv.h>

struct config
{
    item_t p;
    char *data;
};

struct config parsefile();
void freeconfig(struct config cfg);
char *kv_strdefault(item_t p, char *k, char *d);

#endif
