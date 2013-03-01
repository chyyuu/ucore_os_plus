#include <stdio.h>
#include <stdlib.h>
#include <elf.h>
#include <endian.h>
#include <unistd.h>
#include <getopt.h>

#include "e820.h"

#define ELF_MAGIC 0x464c457f
#define MEMORY    0x400000	// 4M
#define FS_BASE   MEMORY
#define FS_SIZE   0x800000	// 8M

#define BUFFER_SIZE 2048
char buffer[BUFFER_SIZE];

#define EMPTY_SIZE 0x100

int main(int argc, char *argv[])
{
	if (argc < 3) {
		perror
		    ("usage: or32-elf-loader <executable> <output> [-f <fs img>]\n");
		exit(1);
	}
	int opt;
	char *fsimg = NULL;
	if ((opt = getopt(argc, argv, "f:")) != -1) {
		fsimg = optarg;
	}

	FILE *input = fopen(argv[1], "r");
	if (input == NULL) {
		printf("cannot open %s for reading.\n", argv[1]);
		exit(1);
	}

	Elf32_Ehdr header;
	fread(&header, 1, sizeof(Elf32_Ehdr), input);
	if (*((int *)header.e_ident) != ELF_MAGIC) {
		printf("Invalid elf file: %s\n", argv[1]);
		exit(1);
	}

	FILE *output = fopen(argv[2], "w");
	if (output == NULL) {
		printf("cannot open %s for writing.\n", argv[2]);
		exit(1);
	}

	/* Load the elf kernel. */
	uint32_t phnum, phnum_total = be16toh(header.e_phnum);
	struct e820map mem_map;
	int current_entry = 0, mem_top = 0x4000;
	/* The first 4k is reserved for kernel */
	mem_map.map[current_entry].addr = htobe64(0x0);
	mem_map.map[current_entry].size = htobe64(0x4000);
	mem_map.map[current_entry].type = htobe32(E820_ARR);
	current_entry++;
	for (phnum = 0; phnum < phnum_total; phnum++) {
		Elf32_Phdr ph;
		uint32_t phoff =
		    be32toh(header.e_phoff) + sizeof(Elf32_Phdr) * phnum;
		fseek(input, phoff, SEEK_SET);
		fread(&ph, 1, sizeof(Elf32_Phdr), input);

		if (be32toh(ph.p_type) != PT_LOAD)
			continue;
		if (be32toh(ph.p_filesz) > be32toh(ph.p_memsz)) {
			printf("Invalid elf\n");
			exit(2);
		}
		if (ph.p_filesz == 0)
			continue;

		uint32_t source = be32toh(ph.p_offset), dest =
		    be32toh(ph.p_paddr);
		uint32_t end = source + be32toh(ph.p_filesz), memsize =
		    ROUNDUP(be32toh(ph.p_memsz), PGSIZE);
		if (source == 0) {	/* Elf header should not load to the RAM */
			source += EMPTY_SIZE;
			dest += EMPTY_SIZE;
			if (source >= end)
				continue;
		} else {
			mem_map.map[current_entry].addr =
			    htobe64((uint64_t) source);
			mem_map.map[current_entry].size =
			    htobe64((uint64_t) memsize);
			mem_map.map[current_entry].type = htobe32(E820_ARR);
			mem_top = source + memsize;
			current_entry++;
		}
		fseek(input, source, SEEK_SET);
		fseek(output, dest, SEEK_SET);
		while (source < end && dest < MEMORY) {
			uint32_t size =
			    (end - source >
			     BUFFER_SIZE) ? BUFFER_SIZE : (end - source);
			fread(buffer, 1, size, input);
			fwrite(buffer, 1, size, output);
			source += size, dest += size;
		}
	}
	if (mem_top < MEMORY) {
		mem_map.map[current_entry].addr = htobe64(mem_top);
		mem_map.map[current_entry].size = htobe64(MEMORY - mem_top);
		mem_map.map[current_entry].type = htobe32(E820_ARM);
	}

	/* Write memory layout to the first 256 bytes */
	mem_map.nr_map = htobe32(current_entry + 1);
	fseek(output, 0, SEEK_SET);
	fwrite(&mem_map, 1, sizeof(mem_map), output);

	/* Load file system to 0x800000 */
	if (fsimg != NULL) {
		FILE *fs = fopen(fsimg, "r");
		if (fs != NULL) {
			fseek(output, FS_BASE, SEEK_SET);
			int fs_ptr = 0;
			while (fs_ptr < FS_SIZE) {
				uint32_t size =
				    fread(buffer, 1, BUFFER_SIZE, fs);
				fwrite(buffer, 1, size, output);
				if (size < BUFFER_SIZE) {
					break;
				}
				fs_ptr += BUFFER_SIZE;
			}
			if (fread(buffer, 1, 1, fs) > 0) {
				printf
				    ("WARNING: file system image too large.\n");
			}
			fclose(fs);
		}
	}

	fclose(input);
	fclose(output);

	return 0;
}
