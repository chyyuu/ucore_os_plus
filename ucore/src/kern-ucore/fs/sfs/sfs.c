#include <types.h>
#include <sfs.h>
#include <error.h>
#include <assert.h>

void
sfs_init(void) {
    int ret;
    if ((ret = sfs_mount("disk0")) != 0) {
        panic("failed: sfs: sfs_mount: %e.\n", ret);
    }
}

