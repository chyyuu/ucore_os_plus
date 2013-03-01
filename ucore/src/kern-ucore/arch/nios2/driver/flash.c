#include <flash.h>
#include <system.h>
#include <assert.h>
#include <io.h>
#include <stdio.h>
#include <fs.h>
#include <sem.h>
#include <clock.h>

#define FLASH_BEGIN_ADDR  0x100000

#define FLASH_ADDR(addr) (0xE0000000 + CFI_FLASH_BASE + FLASH_BEGIN_ADDR + (addr))

#define IS_ERASED(addr) (*((uint8_t*)(addr)) == 0xFF)

#define BLOCK_SIZE 0x10000

static struct ide_device {
	//unsigned char valid;        // 0 or 1 (If Device Really Exists)
	//unsigned int sets;          // Commend Sets Supported
	unsigned int size;	// Size in Sectors
	//unsigned char model[41];    // Model in String
	semaphore_t sem;
} flash;

static void lock_flash(void)
{
	down(&(flash.sem));
}

static void unlock_flash(void)
{
	up(&(flash.sem));
}

size_t flash_size()
{
	return flash.size;
}

static inline uint8_t flash_read(uint32_t flash_addr)
{
	return IORD_8DIRECT(0xE0000000 + CFI_FLASH_BASE, flash_addr);
}

static void erase(uint32_t flash_addr)
{
	IOWR_8DIRECT(0xE0000000 + CFI_FLASH_BASE, 0xAAA, 0xAA);
	IOWR_8DIRECT(0xE0000000 + CFI_FLASH_BASE, 0x555, 0x55);
	IOWR_8DIRECT(0xE0000000 + CFI_FLASH_BASE, 0xAAA, 0x80);
	IOWR_8DIRECT(0xE0000000 + CFI_FLASH_BASE, 0xAAA, 0xAA);
	IOWR_8DIRECT(0xE0000000 + CFI_FLASH_BASE, 0x555, 0x55);
	IOWR_8DIRECT(0xE0000000 + CFI_FLASH_BASE, flash_addr, 0x30);

	usleep(10000);

	uint8_t value;
	do {
		value = IORD_8DIRECT(0xE0000000 + CFI_FLASH_BASE, flash_addr);
		usleep(1000);
	} while (value != 0xFF);
}

static inline void flash_write(uint32_t flash_addr, uint8_t data)
{
	IOWR_8DIRECT(0xE0000000 + CFI_FLASH_BASE, 0xAAA, 0xAA);
	IOWR_8DIRECT(0xE0000000 + CFI_FLASH_BASE, 0x555, 0x55);
	IOWR_8DIRECT(0xE0000000 + CFI_FLASH_BASE, 0xAAA, 0xA0);
	IOWR_8DIRECT(0xE0000000 + CFI_FLASH_BASE, flash_addr, data);
	uint8_t value;
	do {
		value = IORD_8DIRECT(0xE0000000 + CFI_FLASH_BASE, flash_addr);
		if (((value & 0x80) == (data & 0x80)) || (value & 0x20))
			break;
	} while (1);
}

static void test_write()
{
	uint32_t base = 0xa0000;
	flash_write(base + 2, 0xcc);
	int i;
	for (i = 0; i < 10; ++i) {
		uint32_t addr = base + i;
		kprintf("test_write : [0x%x]=0x%x\n", addr, flash_read(addr));
	}
}

void flash_init(void)
{
	flash.size =
	    (CFI_FLASH_END + 1 - CFI_FLASH_BASE - FLASH_BEGIN_ADDR) / SECTSIZE;

	sem_init(&(flash.sem), 1);
}

int flash_read_secs(uint32_t secno, void *dst, size_t nsecs)
{
	lock_flash();

	uint8_t *from = (uint8_t *) FLASH_ADDR(secno * SECTSIZE);
	uint8_t *to = (uint8_t *) dst;
	int i;
	for (i = 0; i < nsecs * SECTSIZE; ++i) {
		*to = *from;
		++to;
		++from;
	}

	unlock_flash();
	return 0;
}

static inline int block_id(uint32_t flash_addr)
{
	return (flash_addr) >> 16;
}

static bool
need_erase(uint32_t flash_addr_st, uint32_t flash_addr_ed, uint8_t * src)
{
	uint32_t i;
	for (i = flash_addr_st; i < flash_addr_ed; ++i) {
		uint8_t data = *(src + i - flash_addr_st);
		uint8_t value = flash_read(i);
		if ((data & value) != data)
			return 1;
	}
	return 0;
}

static uint8_t data[BLOCK_SIZE];
static void
erase_and_write(uint32_t flash_addr_st, uint32_t flash_addr_ed, uint8_t * src)
{
	uint32_t i;
	uint32_t block_base = flash_addr_st & ~0xFFFF;
	for (i = 0; i < BLOCK_SIZE; ++i) {
		data[i] = flash_read(block_base + i);
	}
	for (i = flash_addr_st; i < flash_addr_ed; ++i)
		data[i & 0xFFFF] = src[i - flash_addr_st];

	//do erase
	erase(block_base);
	for (i = 0; i < BLOCK_SIZE; ++i) {
		flash_write(block_base + i, data[i]);
	}
}

static void check_write(uint32_t base, uint32_t size, uint8_t * src)
{
	uint32_t i;
	for (i = 0; i < size; ++i) {
		uint8_t value = flash_read(base + i);
		uint8_t data = src[i];
		if (value != data)
			panic("check write failed!!! failed_addr=0x%x\n",
			      base + i);
	}
}

int flash_write_secs(uint32_t secno, void *src, size_t nsecs)
{
	lock_flash();

	int size = nsecs * SECTSIZE;
	uint32_t base = secno * SECTSIZE + FLASH_BEGIN_ADDR;
	uint32_t st, ed;
	for (st = base; st < base + size; st = ed) {
		ed = st + 1;
		while (ed < base + size && block_id(ed) == block_id(st))
			++ed;
		if (need_erase(st, ed, (uint8_t *) (src + st - base))) {
			erase_and_write(st, ed, (uint8_t *) (src + st - base));
		} else {
			uint32_t i;
			for (i = st; i < ed; ++i) {
				flash_write(i, *(uint8_t *) (src + i - base));
			}
		}
	}
	check_write(base, size, (uint8_t *) src);

	unlock_flash();
	return 0;
}
