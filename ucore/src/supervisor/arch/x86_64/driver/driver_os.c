#include <driver_os.h>
#include <lapic.h>
#include <sysconf.h>
#include <string.h>
#include <trap.h>
#include <stdio.h>
#include <lcpu.h>
#include <memlayout.h>

volatile char *driver_os_buffer;
size_t driver_os_buffer_size;

void driver_os_init(void)
{
#if HAS_DRIVER_OS
	driver_os_buffer_size = DRIVER_OS_BUFFER_PSIZE << PGSHIFT;
	driver_os_buffer = page2va(alloc_pages(DRIVER_OS_BUFFER_PSIZE));

	extern volatile int lcpu_init_count;

	/* Set up for the Linux driver OS */
	while (lcpu_init_count < sysconf.lcpu_count - 2) ;
	extern char _binary_bzImage_start[];
	extern char _binary_bzImage_end[];

	/* Consider the boot protocal */
	/* ALL THE MAGIC FROM BOOT.TXT */
	struct {
		uint8_t setup_sects;
		uint16_t root_flags;
		uint32_t syssize;
		uint16_t ram_size;
#define RAMDISK_IMAGE_START_MASK	0x07FF
#define RAMDISK_PROMPT_FLAG		    0x8000
#define RAMDISK_LOAD_FLAG		    0x4000
		uint16_t vid_mode;
		uint16_t root_dev;
		uint16_t boot_flag;
		uint16_t jump;
		uint32_t header;
		uint16_t version;
		uint32_t realmode_swtch;
		uint16_t start_sys;
		uint16_t kernel_version;
		uint8_t type_of_loader;
		uint8_t loadflags;
#define LOADED_HIGH	    (1<<0)
#define QUIET_FLAG	    (1<<5)
#define KEEP_SEGMENTS	(1<<6)
#define CAN_USE_HEAP	(1<<7)
		uint16_t setup_move_size;
		uint32_t code32_start;
		uint32_t ramdisk_image;
		uint32_t ramdisk_size;
		uint32_t bootsect_kludge;
		uint16_t heap_end_ptr;
		uint8_t ext_loader_ver;
		uint8_t ext_loader_type;
		uint32_t cmd_line_ptr;
		uint32_t initrd_addr_max;
		uint32_t kernel_alignment;
		uint8_t relocatable_kernel;
		uint8_t _pad2[3];
		uint32_t cmdline_size;
		uint32_t hardware_subarch;
		uint64_t hardware_subarch_data;
		uint32_t payload_offset;
		uint32_t payload_length;
		uint64_t setup_data;
	} __attribute__ ((packed)) * hdr;

	extern char __boot_dos_entry[];
	uintptr_t boot_dos_entry =
	    (uintptr_t) __boot_dos_entry + 0x1F8000 + SVBASE;

	memmove(VADDR_DIRECT(0x8000), (void *)boot_dos_entry, 0x8000);
	memmove(VADDR_DIRECT(0x10000), _binary_bzImage_start, 0x8000);

#define CMDLINE_SIZE 400
	char cmdline[CMDLINE_SIZE];
	snprintf(cmdline, CMDLINE_SIZE,
		 /* XXX magic cmdline */
		 "nosmp mem=%dM WITH_DOSM=y DOSM_BA=%p DOSM_BS=%d DOSM_IPIV=%d",
		 RESERVED_DRIVER_OS_SIZE >> 20,
		 PADDR_DIRECT(driver_os_buffer), driver_os_buffer_size,
		 T_IPI_DOS);
	cmdline[CMDLINE_SIZE - 1] = 0;
	strcpy(VADDR_DIRECT(0x1e000), cmdline);

	hdr = (void *)0x101F1;
	hdr->type_of_loader = 0xFF;
	hdr->loadflags = LOADED_HIGH | CAN_USE_HEAP;
	// hdr->setup_move_size = !!! ignored for high version boot protocal
	hdr->ramdisk_size = 0;
	hdr->cmd_line_ptr = 0x1e000;
	hdr->heap_end_ptr = 0xe000 - 0x200;

	uintptr_t start = ((hdr->setup_sects ? hdr->setup_sects : 4) + 1) * 512;
	memmove(VADDR_DIRECT(0x100000), _binary_bzImage_start + start,
		_binary_bzImage_end - _binary_bzImage_start - start);

	lapic_ap_start(lcpu_id_set[sysconf.lcpu_count - 1], 0x8000);
#endif
}

/* MAGIC FROM THE X86_PLATFORM_IPI_VECTOR in arch/x86/include/asm/irq_vectors.h */
#define X86_PLATFORM_IPI_VECTOR 0xf7

void driver_os_notify(void)
{
#if HAS_DRIVER_OS
	lapic_ipi_issue_spec(lcpu_id_set[sysconf.lcpu_count - 1],
			     X86_PLATFORM_IPI_VECTOR);
#endif
}
