#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <elf.h>
#include <getopt.h>

#define SECTSIZE        512	// bytes of a sector
#define PAGE_NSECT      (PGSIZE / SECTSIZE)	// sectors per page

/* We don't need anything more than macros and struct declarations */
#define __BOOT__
#include <glue_memlayout.h>
#undef __BOOT__
#include <boot.h>

struct option longopts[] = {
	{"help", no_argument, NULL, 'h'},
	{"swap", required_argument, NULL, 's'},
	{"disk", required_argument, NULL, 'd'},
	{"kernel", required_argument, NULL, 'k'},
	{"memsize", required_argument, NULL, 'm'},
	{"debug", no_argument, NULL, 'x'},
	{0, 0, 0, 0},
};

int memsize = 0x4000000;

/**
 * Create virtual physical memory in /tmp
 * @param size the size of memory
 */
static int create_mem_file(int size)
{
	char tempname[] = "/tmp/vpmXXXXXX";
	int fd;

	fd = mkstemp(tempname);
	if (fd < 0) {
		perror("create temp file failed.\n");
		_exit(1);
	}
	if (unlink(tempname) < 0) {
		perror("unlink failed\n");
		_exit(1);
	}

	/* Expand the file to the desired length */
	printf("memory size: 0x%08x\n", size);
	if (lseek(fd, size - 1, SEEK_SET) < 0) {
		perror("lseek failed\n");
		_exit(1);
	}
	char zero = 0;
	if (write(fd, &zero, 1) < 0) {
		perror("write failed\n");
		_exit(1);
	}

	/* Change permission of the file */
	if (fchmod(fd, 0777) < 0) {
		perror("fchmod failed\n");
		_exit(1);
	}
	fcntl(fd, F_SETFD, FD_CLOEXEC);

	return fd;
}

/**
 * make a copy of the kernel in vpm
 * @param kernel_fd descriptor of the kernel file
 * @param vpm_offset offset of the vpm mapped
 */
static void read_kernel(int kernel_fd, int vpm_offset)
{
	Elf32_Ehdr header;
	read(kernel_fd, &header, sizeof(Elf32_Ehdr));
	int *magic = (int *)(&(header.e_ident));	/* just to shut gcc up */
	if (*magic != 0x464c457f) {	/* 0x464c457f is the magic number */
		printf("Invalid elf file.\n");
		_exit(1);
	}
	Elf32_Phdr ph;
	int ph_offset = header.e_phoff, i;
	for (i = 0; i < header.e_phnum; i++, ph_offset += sizeof(Elf32_Phdr)) {
		/* read a segment header */
		lseek(kernel_fd, ph_offset, SEEK_SET);
		read(kernel_fd, &ph, sizeof(Elf32_Phdr));
		/* copy data from elf file to vpm */
		if (ph.p_memsz > 0) {
			lseek(kernel_fd, ph.p_offset, SEEK_SET);
			read(kernel_fd, (void *)(ph.p_vaddr + vpm_offset),
			     ph.p_filesz);
		}
	}
}

/**
 * initialize simulated 'e820map'
 * should be called after the reserved memory is mapped
 * @param kernel_fd descriptor of the kernel file
 */
static void e820_init(int kernel_fd)
{
	Elf32_Ehdr header;
	lseek(kernel_fd, 0, SEEK_SET);
	read(kernel_fd, &header, sizeof(Elf32_Ehdr));
	Elf32_Phdr ph;
	//struct e820map* memmap = (struct e820map*)E820MAP;

	/*
	 * map[0...n-1]: kernel segments in the header
	 * map[n]: free memory part
	 */
	int ph_offset = header.e_phoff, i, n = 0;
	int kernel_end = 0;
	/* Reserved part */
	e820map.map[n].addr = KERNBASE;
	e820map.map[n].size = RESERVED_SIZE;
	e820map.map[n].type = E820_ARR;
	n++;
	/* Kernel code and data */
	for (i = 0; i < header.e_phnum; i++, ph_offset += sizeof(Elf32_Phdr)) {
		/* read a segment header */
		lseek(kernel_fd, ph_offset, SEEK_SET);
		read(kernel_fd, &ph, sizeof(Elf32_Phdr));
		if (ph.p_memsz > 0) {
			e820map.map[n].addr = ph.p_paddr;
			e820map.map[n].size = ROUNDUP_PAGE(ph.p_memsz);
			e820map.map[n].type = E820_ARR;	/* not used */
			if (ROUNDUP_PAGE(ph.p_paddr + ph.p_memsz) > kernel_end)
				kernel_end =
				    ROUNDUP_PAGE(ph.p_paddr + ph.p_memsz);
			n++;
		}
	}
	/* free memory starts from the next page of the end of kernel */
	e820map.map[n].addr = kernel_end;
	e820map.map[n].size = memsize - kernel_end + KERNBASE;
	e820map.map[n].type = E820_ARM;
	e820map.nr_map = n + 1;

	printf("============ memory map ============\n");
	for (i = 0; i < e820map.nr_map; i++) {
		printf("[%d]: addr = 0x%x size = 0x%x\n",
		       i, (int)e820map.map[i].addr, (int)e820map.map[i].size);
	}
	printf("========== memory map end ==========\n");
}

struct temp_stack *stk;

/**
 * initialize data for basic operations after unmap
 */
static void stk_init(int brk_end, int fd)
{
	stk = (void *)(RESERVED_END - sizeof(struct temp_stack));
	stk->addr = RESERVED_END;
	stk->length = memsize - RESERVED_SIZE;
	stk->prot = PROT_READ | PROT_WRITE | PROT_EXEC;
	stk->flags = MAP_SHARED;
	stk->fd = fd;
	stk->offset = RESERVED_SIZE;
}

void parse_opts(int argc, char *argv[])
{
	ginfo->disk_fd = ginfo->swap_fd = -1;
	ginfo->status = STATUS_RUN;

	char c;
	int tmp;
	struct stat buf;

	optind = 1;
	while ((c = getopt_long(argc, argv, ":sdx:", longopts, 0)) != -1) {
		switch (c) {
		case 's':
			tmp = open(optarg, O_RDWR);
			if (tmp < 0) {
				printf("swap file %s not found!\n", optarg);
				_exit(1);
			}
			if (fstat(tmp, &buf) < 0) {
				printf("cannot get swap file status!\n");
				_exit(1);
			}
			ginfo->swap_fd = tmp;
			ginfo->swap_size = (int)(buf.st_size / SECTSIZE);
			break;
		case 'd':
			tmp = open(optarg, O_RDWR);
			if (tmp < 0) {
				printf("disk file %s not found!\n", optarg);
				_exit(1);
			}
			if (fstat(tmp, &buf) < 0) {
				printf("cannot get disk file status!\n");
				_exit(1);
			}
			ginfo->disk_fd = tmp;
			ginfo->disk_size = (int)(buf.st_size / SECTSIZE);
			break;
		case 'x':
			ginfo->status = STATUS_DEBUG;
			break;
		default:
			/* ignore 'help' */
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	int fd, kernel_fd = -1;

	/* scanning whether there's -h or --help in the options */
	char c, *unit;
	while ((c = getopt_long(argc, argv, ":skmdx:", longopts, 0)) != -1) {
		switch (c) {
		case 'h':
			printf
			    ("  -d <path>, --disk=<path>    specify normal disk\n"
			     "  -k <path>, --kernel=<path>  specify kernel file\n"
			     "  -m NUM   , --memsize=NUM    specify memory size\n"
			     "  -s <path>, --swap=<path>    specify swap disk\n"
			     "  -h, --help                  print this message and exit\n");
			return 0;
			break;
		case 'k':
			if ((kernel_fd = open(optarg, O_RDONLY)) < 0)
				return 1;
			break;
		case 'm':
			memsize = strtol(optarg, &unit, 10);
			if (*unit == 'K' || *unit == 'k')
				memsize <<= 10;
			else if (*unit == 'M' || *unit == 'm')
				memsize <<= 20;
			break;
		default:
			/* ignore other options now */
			break;
		}
	}
	if (kernel_fd < 0) {
		printf("No kernel file specified.\n");
		return 1;
	}

	struct stat buf;
	fstat(kernel_fd, &buf);

	fd = create_mem_file(memsize);

	/* map the temp file & copy boot and kernel code to the memory */
	int brk_end = (int)sbrk(0);
	mmap((void *)brk_end, memsize, PROT_READ | PROT_WRITE | PROT_EXEC,
	     MAP_SHARED, fd, 0);

	read_kernel(kernel_fd, brk_end - KERNBASE);

	/* map the reserved part to where it really should be */
	mmap((void *)KERNBASE, RESERVED_SIZE,
	     PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED, fd, 0);

	/* Now we have access to ginfo normally. */
	ginfo->mem_fd = fd;
	e820_init(kernel_fd);
	parse_opts(argc, argv);

	/* unmap useless page table entries */
	memcpy((void *)(BOOT_CODE), (void *)&__boot_start, PGSIZE);
	stk_init(brk_end, fd);

	void (*boot_start) (void);
	boot_start = __pa(__boot_entry);
	boot_start();

	return 0;
}

/**
 * __boot_entry will call this function after flushing the space
 */
void __attribute__ ((__section__(".__boot")))
    boot_main(void)
{
	Elf32_Ehdr *header = (Elf32_Ehdr *) RESERVED_END;

	((void (*)(void))(header->e_entry)) ();

	asm("movl  $1, %eax; movl  $0, %ebx; int $0x80;");
}
