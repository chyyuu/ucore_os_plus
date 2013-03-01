/**************************************************
 * This is the C part of the or32-ucore on-disk loader.
 * Try figure out where ucore is and load it to the memory.
 * Note: The sdcard should be accessable at present.
 **************************************************/

#include <elf.h>
#include <board.h>
#include <gpio.h>
#include <spi.h>
#include <or32.h>

/**
 * Read a block from the sd card.
 * @param dst     the buffer where the data read is placed.
 * @param blkno   the block number
 * @return        0 on success, or -1 otherwise.
 */
extern void _readblock(void *dst, uint32_t blkno);

void bootmain(struct partition_info_entry *entries) __attribute__ ((noreturn));

/**
 * Read a whole segment from the sd card.
 * @param pa      the physical address where the place the segment
 * @param count   the size of the segment (in byte)
 * @param start   the start position of the kernel elf (in block)
 * @param offset  the offset of the segment in the elf file (in byte)
 */
static void
readseg(uintptr_t pa, uint32_t count, uint32_t start, uint32_t offset)
{
	uintptr_t end_pa = pa + count;

	/* Round down to block boundary. */
	pa -= offset % BLOCK_SIZE;

	/* Translate from bytes to blocks. */
	uint32_t blkno = (offset / BLOCK_SIZE) + start;

	for (; pa < end_pa; pa += BLOCK_SIZE, blkno++)
		_readblock((void *)pa, blkno);
}

void bootmain(struct partition_info_entry *entries)
{
	int err;

	/* Find the bootable partition (that should be where ucore is). */
	int i;
	for (i = 0; i < 4; i++, entries++) {
		if (entries->flag & PARTITION_ENTRY_FLAG_BOOTABLE)
			break;
	}

	/* Load the elf header. */
	uint8_t __hdr_buf[512];
	struct elfhdr *elf_hdr = (struct elfhdr *)&__hdr_buf;
	uint32_t start_blk = entries->start;
	_readblock(elf_hdr, start_blk);

	/* Examine whether it's a valid elf file. */
	if (elf_hdr->e_magic != ELF_MAGIC) {
		goto fail;
	}

	struct proghdr *ph, *eph;

	/* load each program segment (ignores ph flags) */
	ph = (struct proghdr *)((uintptr_t) elf_hdr + elf_hdr->e_phoff);
	eph = ph + elf_hdr->e_phnum;
	for (; ph < eph; ph++) {
		readseg(ph->p_pa, ph->p_memsz, start_blk, ph->p_offset);
	}

	// call the entry point from the ELF header
	// note: does not return
	((void (*)(void))(elf_hdr->e_entry)) ();

fail:
	REG32(GPIO_PHYSICAL_BASE + GPIO_OUTPUT) = 0xdead;
	while (1) ;
}
