/*
 * =====================================================================================
 *
 *       Filename:  kshm_test.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/29/2012 10:35:38 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */
#include <ulib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stat.h>
#include <file.h>
#include <dir.h>
#include <unistd.h>

#define printf(...)                 fprintf(1, __VA_ARGS__)
#define KSHM_BASE 0xDF000000
#define KSHM_SIZE (1<<20)	/* 1M */

#define MIN(x,y) ((x)<(y)?(x):(y))

static unsigned char buf[512];

#define KSHM_SB_MAGIC 0x48594853
#define MAX_FN_LEN 128
struct kshm_superblk {
	uint32_t magic;
	uint32_t filesz;
	uint32_t checksum;
	char filename[0];
};

int main(int argc, char *argv[])
{
	void *kshm_base = (void *)KSHM_BASE;
	struct kshm_superblk *sb = NULL;
	uint32_t a = *(uint32_t *) kshm_base;
	assert(kshm_base != NULL);
	printf("[0]: %08x\n", a);
	/* KSHM_BASE is in kernel space, copy to user space first */
	memcpy(buf, kshm_base, 512);
	sb = (struct kshm_superblk *)buf;
	if (sb->magic != KSHM_SB_MAGIC) {
		printf("ERROR: invalid magic 0x%08x\n", sb->magic);
		return -1;
	}
	if (sb->filesz > (1 << 19)) {	//512k
		printf("ERROR: file too big 0x%08x\n", sb->filesz);
		return -1;
	}
	printf("Filename: %s\n", sb->filename);
	printf("Filesize: %d\n", sb->filesz);

	int fd = open(sb->filename, O_WRONLY | O_TRUNC | O_CREAT);
	if (fd < 0) {
		printf("failed to open file.\n");
		return fd;
	}

	int rdcnt = 0;
	int leftsize = sb->filesz;
	int filesize = sb->filesz;
	while (rdcnt < filesize) {
		//printf("rdcnt: %d\n", rdcnt);
		int len = 0;
		memcpy(buf, kshm_base + 512 + rdcnt, 512);
		rdcnt += 512;
		len = MIN(512, leftsize);
		leftsize -= len;
		len = write(fd, buf, len);
		if (len < 0)
			return len;
	}
	close(fd);
	return 0;
}
