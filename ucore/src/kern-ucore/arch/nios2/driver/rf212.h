#ifndef __NIOS2_DRIVER_RF212_H__
#define __NIOS2_DRIVER_RF212_H__

void rf212_init();
void rf212_reset();
void rf212_send(uint8_t len, uint8_t * data);
void rf212_reg(uint8_t reg, uint8_t value);

#endif
