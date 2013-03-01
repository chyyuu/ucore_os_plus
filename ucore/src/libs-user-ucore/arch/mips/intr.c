#include <arch.h>
#include <intr.h>
#include <asm/mipsregs.h>

#define get_status(x) __asm volatile("mfc0 %0,$12" : "=r" (x))
#define set_status(x) __asm volatile("mtc0 %0,$12" :: "r" (x))

/* intr_enable - enable irq interrupt */
void intr_enable(void)
{
	uint32_t x;
	get_status(x);
	x |= ST0_IE;
	set_status(x);

}

/* intr_disable - disable irq interrupt */
void intr_disable(void)
{
	uint32_t x;
	get_status(x);
	x &= ~ST0_IE;
	set_status(x);
}
