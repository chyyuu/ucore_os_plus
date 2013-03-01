#include <types.h>
#include <sem.h>
#include <sfs.h>

void lock_sfs_fs(struct sfs_fs *sfs)
{
	down(&(sfs->fs_sem));
}

void lock_sfs_io(struct sfs_fs *sfs)
{
	down(&(sfs->io_sem));
}

void lock_sfs_mutex(struct sfs_fs *sfs)
{
	down(&(sfs->mutex_sem));
}

void unlock_sfs_fs(struct sfs_fs *sfs)
{
	up(&(sfs->fs_sem));
}

void unlock_sfs_io(struct sfs_fs *sfs)
{
	up(&(sfs->io_sem));
}

void unlock_sfs_mutex(struct sfs_fs *sfs)
{
	up(&(sfs->mutex_sem));
}
