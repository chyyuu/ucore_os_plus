#include <arm.h>
#include <intr.h>

/* intr_enable - enable irq interrupt */
void intr_enable(void)
{
	irq_flag_enable();
}

/* intr_disable - disable irq interrupt */
void intr_disable(void)
{
	irq_flag_disable();
}
