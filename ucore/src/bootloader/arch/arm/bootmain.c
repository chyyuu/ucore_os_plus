/*
 *  bootmain.c
 *  copy kernel image to ram
 *  kernel should be placed in first sector of SROM, which is INITIAL_LOAD
 *
 *  author: YU KuanLong / HE Alexis
 *  part of the code is written by WANG JianFei
 *  date:   2011-3-21
 */

#include <types.h>
#include <elf.h>
#include <board.h>

#define ELFHDR		((struct elfhdr *) (BOOTLOADER_BASE+0x1000))	// scratch space

/* uart_putc, uart_put_s
 * write on UART0 */
static void uart_putc(int c)
{
#ifdef PLATFORM_AT91
#define UART0_CSR ((volatile unsigned int*)(AT91SAM_DBGU_BASE+DBGU_CSR))
	while (!((*UART0_CSR) & AT91C_US_TXRDY)) ;
	*UART0_TX = c;
#endif

#ifdef PLATFORM_VERSATILEPB
	*UART0_TX = c;
#endif

#ifdef PLATFORM_GOLDFISH
	*UART0_TX = c;
#endif

#ifdef PLATFORM_RASPBERRYPI
	while (!((*UART0_CSR) & RASPI_US_TXRDY)) ;
	*UART0_TX = c;
#endif
}

static void uart_puts(const char *p)
{
	while (*p)
		uart_putc(*p++);
}

/* *
 * readseg - read @count bytes at @offset from kernel into virtual address @va,
 * might copy more than asked.
 * */
static void readseg(uintptr_t va, uint32_t count, uint32_t offset)
{
	__memcpy((void *)va, (void *)((uint8_t *) ELFHDR + offset), count);
}

/* bootmain - the entry of bootloader */
void bootmain(void)
{

	uart_puts("Booting...\n");

	// is this a valid ELF?
	if (ELFHDR->e_magic != ELF_MAGIC) {
		goto bad;
	}

	struct proghdr *ph, *eph;

	// load each program segment (ignores ph flags)
	ph = (struct proghdr *)((uintptr_t) ELFHDR + ELFHDR->e_phoff);
	eph = ph + ELFHDR->e_phnum;
	for (; ph < eph; ph++) {
		readseg(ph->p_va, ph->p_memsz, ph->p_offset);
	}

	// call the entry point from the ELF header
	// note: does not return
	((void (*)(void))(ELFHDR->e_entry)) ();

bad:
	uart_puts("Error.\n");

	/* do nothing */
	while (1) ;
}
