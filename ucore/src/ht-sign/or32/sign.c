#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <endian.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

#define BOOTBLOCK_MAX_SIZE   446
#define BLOCK_SIZE           512

struct partition_info_entry {
	uint8_t flag;
	uint8_t padding1;
	uint32_t start;		/* in block */
	uint32_t size;		/* in block */
	uint32_t padding2;
	uint16_t padding3;
} __attribute__ ((packed));

#define PARTITION_UNIT                0x8000	/* The size of a partition is a multiple of this (in block) */
#define PARTITION_ENTRY_FLAG_BOOTABLE 0x80	/* The block contains a valid kernel elf. */
#define PARTITION_ENTRY_FLAG_VALID    0x1

char buffer[BLOCK_SIZE];

int main(int argc, char *argv[])
{
	if (argc < 3) {
		printf
		    ("usage: %s <dev/file name> <bootblock> [<part1> <part2> ...]\n",
		     argv[0]);
		return 1;
	}

	int disk = open(argv[1], O_WRONLY | O_CREAT);
	if (disk < 0) {
		printf("cannot open dev: %s\n", argv[1]);
		return 1;
	}

	int bootblock = open(argv[2], O_RDONLY);
	if (bootblock < 0) {
		printf("cannot open bootblock file: %s\n", argv[2]);
		return 1;
	}

	int i;
	/* At most afford four partitions at present (i.e. primary partitions only).
	 * and one is reserved for swap which is 128M bytes or 256K blocks.
	 */
	int parts[4] = { 0, 0, 0, 0 };
	for (i = 3; i < argc; i++) {
		if (i >= 6) {
			printf
			    ("warning: too much partitions. only the first four are handled.");
			break;
		}
		if ((parts[i - 3] = open(argv[i], O_RDONLY)) < 0) {
			printf("cannot open file: %s\n", argv[i]);
			return 1;
		}
	}

	/* All files are opened correctly. Let's do the work. */
	struct stat file_status;
	struct partition_info_entry entries[4];
	uint32_t next_block = 1, end, nr_unit;	/* Block 0 is reserved for the bootblock */
	uint32_t actual_blocks;

	/* Copy the bootblock. */
	if (fstat(bootblock, &file_status) < 0) {
		printf("cannot get status of the bootblock\n");
		return 1;
	}
	if (file_status.st_size > BOOTBLOCK_MAX_SIZE) {
		printf("bootblock is %d bytes, which is too LARGE!!!\n",
		       (uint32_t) file_status.st_size);
		return 1;
	}
	lseek(disk, 0, SEEK_SET);
	read(bootblock, &buffer, file_status.st_size);
	write(disk, &buffer, file_status.st_size);

	/* Prepare for the partition table and copy the files by the way. */
	memset(&entries, 0, sizeof(entries));
	for (i = 0; i < 4; i++) {
		if (parts[i] != 0) {
			if (fstat(parts[i], &file_status) < 0) {
				printf("cannot get status of the file %d.", i);
				return 1;
			}
			if (file_status.st_size == 0)	/* What's this file? */
				continue;
			entries[i].flag |= PARTITION_ENTRY_FLAG_VALID;
			nr_unit =
			    (next_block * BLOCK_SIZE + file_status.st_size -
			     1) / (PARTITION_UNIT * BLOCK_SIZE) + 1;
			end = nr_unit * PARTITION_UNIT;
			entries[i].start = next_block;
			entries[i].size = end - next_block;
			next_block = end;

			/* Now copy the files. */
			int j;
			actual_blocks =
			    (file_status.st_size - 1) / BLOCK_SIZE + 1;
			lseek(disk, entries[i].start * BLOCK_SIZE, SEEK_SET);
			for (j = 0; j < actual_blocks; j++) {
				read(parts[i], &buffer, BLOCK_SIZE);
				write(disk, &buffer, BLOCK_SIZE);
			}
		} else {
			/* Swap is 128K blocks, and has no need for initializing. */
			entries[i].flag |= PARTITION_ENTRY_FLAG_VALID;
			end = 0x40000;
			entries[i].start = next_block;
			entries[i].size = end;
			break;
		}
	}

	/* Save the partition table. Note that we have a big-endian system.
	 * First change the endian.
	 */
	entries[0].flag |= PARTITION_ENTRY_FLAG_BOOTABLE;
	for (i = 0; i < 4; i++) {
		entries[i].start = htobe32(entries[i].start);
		entries[i].size = htobe32(entries[i].size);
	}
	/* Second write to the disk */
	lseek(disk, BLOCK_SIZE - 66, SEEK_SET);
	write(disk, (char *)&entries, sizeof(entries));

	/* Finally we can sign the disk as valid. */
	buffer[0] = 0x55;
	buffer[1] = 0xAA;
	write(disk, &buffer, 2);

	/* That's all. Let the OS close all the files for us. */
	printf("The image is created successfully.\n");

	return 0;
}
