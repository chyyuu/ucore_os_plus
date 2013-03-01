#ifndef __NIOS2_TLB_H__
#define __NIOS2_TLB_H__

#include <types.h>

#define PTEADDR_SET_VPN(pteaddr,vpn) ((pteaddr)=((vpn)<<2))

#define TLBACC_C (1<<24)
#define TLBACC_R (1<<23)
#define TLBACC_W (1<<22)
#define TLBACC_X (1<<21)
#define TLBACC_G (1<<20)

#define TLBMISC_RD (1<<19)
#define TLBMISC_WE (1<<18)

#define TLBMISC_SET_PID(tlbmisc, pid) ((tlbmisc)=((tlbmisc)&0xFFFC000F)|((pid<<4)&0x3FFF0))

void tlb_init(void);

//perm = 1 when this is a permission violation exception; else perm = 0.
//if success, return 0
int tlb_miss_handler(uintptr_t la, bool perm);

void tlb_write(uintptr_t la, uintptr_t pa, bool write);

void tlb_setpid(int pid);

#endif /* !__NIOS2_TLB_H__ */
