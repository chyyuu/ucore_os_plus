#include <types.h>
#include <error.h>
#include <assert.h>
#include "fatfs/ffconf.h"
#include "ffs.h"

DWORD get_fattime(void)
{
	return ((DWORD) (2011 - 1980) << 25)
	    | ((DWORD) 3 << 21)
	    | ((DWORD) 26 << 16)
	    | ((DWORD) 19 << 11)
	    | ((DWORD) 28 << 5)
	    | ((DWORD) 0 << 1);
}

void ffs_init()
{
	int ret;
	if ((ret = ffs_mount("mmc0")) != 0) {
		panic("failed: ffs: ffs_mount: %e.\n", ret);
	}
}

#if _FS_REENTRANT

bool ff_cre_syncobj(BYTE _vol, _SYNC_t * sobj)
{
	bool ret = false;
	return ret;
}

bool ff_del_syncobj(_SYNC_t sobj)
{
	bool ret = false;
	return ret;
}

bool ff_req_grant(_SYNC_t sobj)
{
	bool ret = false;
	return ret;
}

void ff_rel_grant(_SYNC_t sobj)
#endif
#if _USE_LFN == 3
void *ff_memalloc(UINT size)
{
	return malloc(size);
}

void ff_memfree(void *mblock)
{
	free(mblock);
}
#endif
