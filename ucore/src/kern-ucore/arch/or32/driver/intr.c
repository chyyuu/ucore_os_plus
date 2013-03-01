#include <intr.h>
#include <arch.h>

/* intr_enable - enable irq interrupt */
void intr_enable(void)
{
	mtspr(SPR_SR, mfspr(SPR_SR) | SPR_SR_IEE | SPR_SR_TEE);
}

/* intr_disable - disable irq interrupt */
void intr_disable(void)
{
	mtspr(SPR_SR, mfspr(SPR_SR) & (~(SPR_SR_IEE | SPR_SR_TEE)));
}
