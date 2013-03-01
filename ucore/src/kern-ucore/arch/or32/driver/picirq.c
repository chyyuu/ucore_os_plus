#include <picirq.h>
#include <arch.h>

void pic_enable(unsigned int irq)
{
	mtspr(SPR_PICMR, mfspr(SPR_PICMR) | (1 << irq));
}

/* pic_init - initialize the programmable interrupt controllers */
void pic_init(void)
{
	pic_enable(IRQ_UART);
	pic_enable(IRQ_TIMER);
}
