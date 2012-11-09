#ifndef __USER_LIBS_RF212_H__
#define __USER_LIBS_RF212_H__

#include <types.h>
#include <syscall.h>
#include <stdio.h>

uint8_t rf212_recv(uint8_t* data)
{
//    return sys_RX(data);
}

void rf212_send(uint8_t len, uint8_t* data)
{
    return sys_rf212_send(len, data);
}
#endif
