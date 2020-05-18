#include "keys.h"
#include <string.h>

wm_key_t wm_str2key(char *_k)
{
	char *k = strdup(_k);
	wm_key_t ks = {0};
	printf("Key %s\n", k);
	char *key = strtok(k, " ");
	while (key)
	{
		printf("\t%s\n", key);
		if (strcmp(key, "shift") == 0)
		{
			ks.mask |= ShiftMask;
		}
		else if (strcmp(key, "mod1") == 0 || strcmp(key, "alt") == 0)
		{
			ks.mask |= Mod1Mask;
		}
		else if (strcmp(key, "mod2") == 0)
		{
			ks.mask |= Mod2Mask;
		}
		else if (strcmp(key, "mod3") == 0)
		{
			ks.mask |= Mod3Mask;
		}
		else if (strcmp(key, "mod4") == 0 || strcmp(key, "super") == 0)
		{
			ks.mask |= Mod4Mask;
		}
		else if (strcmp(key, "mod5") == 0)
		{
			ks.mask |= Mod5Mask;
		}
		else if (strcmp(key, "control") == 0)
		{
			ks.mask |= ControlMask;
		}
		else
		{
			ks.keysym = XStringToKeysym(key);
		}
		key = strtok(NULL, " ");
	}
	free(k);
	printf("wm_key_t %d, %d\n", ks.mask, ks.keysym);
	return ks;
}

bool wm_keyeq(wm_key_t a, wm_key_t b)
{
	return a.keysym == b.keysym && a.mask == b.mask;
}
