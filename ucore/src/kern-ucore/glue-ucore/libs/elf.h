#ifndef __LIBS_ELF_H__
#define __LIBS_ELF_H__

#include <types.h>

#ifdef __BIG_ENDIAN__
#define ELF_MAGIC   0x7F454C46U         // "\x7FELF" in big endian
#else
#define ELF_MAGIC   0x464C457FU         // "\x7FELF" in little endian
#endif

#ifdef __UCORE_64__

/* file header */
struct elfhdr {
    uint32_t e_magic;     // must equal ELF_MAGIC
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

#else /* __UCORE_64__ not defined */

struct elfhdr {
    uint32_t e_magic;     // must equal ELF_MAGIC
    uint8_t e_elf[12];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
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
    uint32_t p_offset;
    uint32_t p_va;
    uint32_t p_pa;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
};

#endif /* __UCORE_64__ */

/* values for Proghdr::p_type */
#define ELF_PT_LOAD                     1

#define ELF_PT_INTERP					3

/* flag bits for Proghdr::p_flags */
#define ELF_PF_X                        1
#define ELF_PF_W                        2
#define ELF_PF_R                        4

/* values for elf's dynamic linker */
#define ELF_AT_NULL						0
#define ELF_AT_EXEFD					2
#define	ELF_AT_PHDR						3
#define ELF_AT_PHENT					4
#define ELF_AT_PHNUM					5
#define ELF_AT_BASE						7
#define ELF_AT_ENTRY					9


#endif /* !__LIBS_ELF_H__ */

