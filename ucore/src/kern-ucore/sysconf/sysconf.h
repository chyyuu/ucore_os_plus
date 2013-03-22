#ifndef __SYSCONF_H__
#define __SYSCONF_H__

#include <types.h>

typedef struct sysconf_s {
	int lcpu_boot;
	int lcpu_count;
	int lnuma_count;

#include <arch_sysconf.h>
} sysconf_s;

extern sysconf_s sysconf;

#endif
