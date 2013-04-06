#ifndef __ARCH_AMD64_MULTIBOOT_H
#define __ARCH_AMD64_MULTIBOOT_H
#include <types.h>
typedef struct Mbdata Mbdata;
struct Mbdata
{
	uint32_t	flags;
	uint32_t	mem_lower;  // flag 0
	uint32_t	mem_upper;  // flag 0
	uint32_t	boot_device;  // flag 1
	uint32_t	cmdline;  // flag 2
	uint32_t	mods_count;  // flag 3
	uint32_t	mods_addr;
	uint32_t	syms[4];  // flag 4, 5
	uint32_t	mmap_length;  // flag 6
	uint32_t	mmap_addr;
	uint32_t	drives_length;  // flag 7
	uint32_t	drives_addr;
	uint32_t	config_table;  // flag 8
	uint32_t	boot_loader_name;  // flag 9
	uint32_t	apm_table;  // flag 10
	uint32_t	vbe_control_info;  // flag 11
	uint32_t	vbe_mode_info;
	uint32_t	vbe_mode;
	uint32_t	vbe_interface_seg;
	uint32_t	vbe_interface_off;
	uint32_t	vbe_interface_len;
};

typedef struct Mbmem Mbmem;
struct Mbmem
{
	uint32_t	size;
	uint64_t	base;
	uint64_t	length;
	uint32_t	type;
} __attribute__((packed));

typedef struct Mbmod Mbmod;
struct Mbmod
{
	uint32_t	start;
	uint32_t	end;
	uint32_t	name;
};
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002
#endif
