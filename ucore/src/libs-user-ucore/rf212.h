#ifndef __USER_LIBS_RF212_H__
#define __USER_LIBS_RF212_H__

#include <types.h>
#include <syscall.h>
#include <stdio.h>

uint8_t rf212_recv(uint8_t * data)
{
//    return sys_RX(data);
}

uint8_t rf212_send(uint8_t len, uint8_t * data)
{
	return sys_rf212_send(len, data);
}

uint8_t rf212_reg(uint8_t reg, uint8_t value)
{
	return sys_rf212_reg(reg, value);
}

uint8_t rf212_reset()
{
	return sys_rf212_reset();
}

#endif
