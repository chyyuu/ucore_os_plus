#ifndef __NIOS2_DRIVER_RF212_H__
#define __NIOS2_DRIVER_RF212_H__

void rf212_init();

void rf212_send(uint8_t len, uint8_t *data);

#endif
