#ifndef __LIBS_ELF_H__
#define __LIBS_ELF_H__

#include <types.h>

#define ELF_MAGIC   0x464C457FU	// "\x7FELF" in little endian

/* file header */
struct elfhdr {
	uint32_t e_magic;	// must equal ELF_MAGIC
	uint8_t e_elf[12];
	uint16_t e_type;
	uint16_t e_machine;
	uint32_t e_version;
	uint64_t e_entry;
	uint64_t e_phoff;
	uint64_t e_shoff;
	uint32_t e_flags;
	uint16_t e_ehsize;
	uint16_t e_phentsize;
	uint16_t e_phnum;
	uint16_t e_shentsize;
	uint16_t e_shnum;
	uint16_t e_shstrndx;
};

/* program section header */
struct proghdr {
	uint32_t p_type;
	uint32_t p_flags;
	uint64_t p_offset;
	uint64_t p_va;
	uint64_t p_pa;
	uint64_t p_filesz;
	uint64_t p_memsz;
	uint64_t p_align;
};

#endif /* !__LIBS_ELF_H__ */
