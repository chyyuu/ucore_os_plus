#ifndef __ARCH_TEMPLATE_INCLUDE_SYNC_H__
#define __ARCH_TEMPLATE_INCLUDE_SYNC_H__

#include <intr.h>
#include <types.h>
#include <arch.h>
#include <assert.h>

/*  return true and disable interrupts if interrupts are enabled at present,
 *      and return false otherwise.
 */
static inline bool __intr_save(void)
{
	uint32_t sr;
	sr = mfspr(SPR_SR);
	unsigned char c = 0;
	if (sr & SPR_SR_IEE)
		c = 1;
	mtspr(SPR_SR, sr & ~(SPR_SR_IEE | SPR_SR_TEE));
	return c;
}

/*! TODO: enable interrupts if @flag if true.
 */
static inline void __intr_restore(bool flag)
{
	if (flag) {
		intr_enable();
	}
}

#define local_intr_save(x)      do { x = __intr_save(); } while (0)
#define local_intr_restore(x)   __intr_restore(x);

void sync_init(void);

#endif /* !__ARCH_TEMPLATE_INCLUDE_SYNC_H__ */
