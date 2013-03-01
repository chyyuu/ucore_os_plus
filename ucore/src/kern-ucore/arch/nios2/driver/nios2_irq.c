#include <types.h>
#include <nios2_irq.h>
#include <nios2.h>
#include <sync.h>

inline int alt_irq_disable(uint32_t id)
{
	int intr_flag;
	local_intr_save(intr_flag);
	{
		uint32_t alt_irq_active;
		NIOS2_READ_IENABLE(alt_irq_active);
		alt_irq_active &= ~(1 << id);
		NIOS2_WRITE_IENABLE(alt_irq_active);
	}
	local_intr_restore(intr_flag);
	return 0;
}

inline int alt_irq_enable(uint32_t id)
{
	int intr_flag;
	local_intr_save(intr_flag);
	{
		uint32_t alt_irq_active;
		NIOS2_READ_IENABLE(alt_irq_active);
		alt_irq_active |= (1 << id);
		NIOS2_WRITE_IENABLE(alt_irq_active);
	}
	local_intr_restore(intr_flag);
	return 0;
}

void irq_init(void)
{
	//did_init = 1;
	NIOS2_WRITE_IENABLE(0);
	NIOS2_WRITE_STATUS(NIOS2_STATUS_PIE_MSK);

}
