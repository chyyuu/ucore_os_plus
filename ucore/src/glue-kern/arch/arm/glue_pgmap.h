#ifndef __ARCH_ARM_INCLUDE_PG_MAP_H__
#define __ARCH_ARM_INCLUDE_PG_MAP_H__

#include <glue_memlayout.h>
#include <mmu.h>

typedef uint32_t pte_perm_t;

// L2 PTE setter
// Set the ucore flags directly, and the hardware flags under condition
// flags are PTE_xxx
static inline void _arm_pte_setflags(pte_t * pte, pte_perm_t flags)
{
	// ucore flags
	*PTE_STATUS(pte) |= flags;
	// hardware flags
	//*pte &= 0xFFFFF000;
	if (flags & PTE_P)
		*pte |= PTEX_L2_PGTYPE;
	*pte &= ~PTEX_PROTECT_MASK;

	if (flags & PTE_U) {
		if (flags & PTE_W)	// Write permission is accorded only if page is writeable and dirty
		{
			*pte |= PTEX_UW;
		} else {
			*pte |= PTEX_U;
		}
	} else {
		if (flags & PTE_W)
			*pte |= PTEX_W;	// kernel will be read/write or no access
		else
			*pte |= PTEX_R;	//kernel readonly
	}

	if (flags & PTE_PWT) {
		*pte &= ~PTEX_CB_MASK;
		*pte |= PTEX_PWT;
	} else if (flags & PTE_IOMEM) {
		*pte &= ~PTEX_CB_MASK;
		*pte |= PTEX_PIO;
	}
}

#define _SET_PTE_BITS(pte, perm) do{pte_perm_t oldf = *(PTE_STATUS(pte));\
                _arm_pte_setflags(pte, oldf|perm);}while(0);

//Assuming cr1 SR = 1 0
static inline void ptep_map(pte_t * ptep, uintptr_t pa)
{
	//*ptep = pa|(PTEX_WB<<2);
	*ptep = pa | (PTEX_PWT << 2);	//write through cache by default
	_arm_pte_setflags(ptep, PTE_P);
}

static inline void pdep_map(pde_t * pdep, uintptr_t pa)
{
	*pdep = pa | PTEX_L1_PDTYPE | (1 << 4);
}

static inline void ptep_unmap(pte_t * ptep)
{
	*ptep = 0;
}

static inline int ptep_invalid(pte_t * ptep)
{
	return (*ptep == 0);
}

static inline int ptep_present(pte_t * ptep)
{
	return (*ptep & PTEX_P);
}

static inline int ptep_s_read(pte_t * ptep)
{
//      return (*ptep & PTEX_W);
	return 1;
}

static inline int ptep_s_write(pte_t * ptep)
{
	uint32_t p = *ptep & PTEX_PROTECT_MASK;
	return (p != PTEX_R);
}

static inline int ptep_s_exec(pte_t * ptep)
{
//      return (*ptep & PTE_E);
	return 1;
}

/* user readable */
static inline int ptep_u_read(pte_t * ptep)
{
	//return (*ptep & PTEX_U);
	uint32_t p = *ptep & PTEX_PROTECT_MASK;
	return (p == PTEX_U) || (p == PTEX_UW);
}

/* user writable */
static inline int ptep_u_write(pte_t * ptep)
{
	uint32_t p = *ptep & PTEX_PROTECT_MASK;
	return (p == PTEX_UW);
	//return (*ptep & PTEX_UW);
}

static inline int ptep_u_exec(pte_t * ptep)
{
//      return (*ptep & PTE_E);
	return ptep_u_read(ptep);
}

static inline void ptep_set_s_read(pte_t * ptep)
{
	//*ptep |= PTE_SPR_R;
	//supervisor mode always readable
}

static inline void ptep_set_s_write(pte_t * ptep)
{
	//if(!ptep_u_read(ptep))
	*ptep |= PTE_W;
	//_SET_PTE_BITS(ptep, PTE_W);
}

static inline void ptep_set_s_exec(pte_t * ptep)
{
	//*ptep |= PTE_E;
}

static inline void ptep_set_u_read(pte_t * ptep)
{
	//_SET_PTE_BITS(ptep, PTE_U);
	*ptep |= PTE_U;
}

static inline void ptep_set_u_write(pte_t * ptep)
{
	//*ptep |= PTEX_UW|PTEX_W;
	//_SET_PTE_BITS(ptep, PTE_U|PTE_W);
	*ptep |= PTE_U | PTE_W;
}

#if 0
static inline void ptep_set_u_exec(pte_t * ptep)
{
//      *ptep |= PTE_E;
	ptep_set_u_read(ptep);
}

static inline void ptep_unset_s_read(pte_t * ptep)
{
//      *ptep &= ~(PTE_SPR_R | PTE_USER_R);
//impossible
}
#endif

static inline void ptep_unset_s_write(pte_t * ptep)
{
//      *ptep &= ~(PTEX_W| PTEX_UW);
// TODO
}

#if 0
static inline void ptep_unset_u_read(pte_t * ptep)
{
	if (ptep_s_write(ptep)) {
		*ptep &= ~PTEX_PROTECT_MASK;
		*ptep |= PTEX_W;
	} else {
		*ptep &= ~PTEX_PROTECT_MASK;
	}
}
#endif

#if 0
static inline void ptep_unset_s_exec(pte_t * ptep)
{
//      *ptep &= ~PTE_E;
//impossible
}

static inline void ptep_unset_u_write(pte_t * ptep)
{
	// *ptep &= ~;
}

static inline void ptep_unset_u_exec(pte_t * ptep)
{
//      *ptep &= ~PTE_E;
//      TODO 
}
#endif

static inline pte_perm_t ptep_get_perm(pte_t * ptep, pte_perm_t perm)
{
	return (*PTE_STATUS(ptep)) & perm;
}

static inline void ptep_set_perm(pte_t * ptep, pte_perm_t perm)
{
	//*PTE_STATUS(ptep) |= perm;
	_arm_pte_setflags(ptep, perm);
}

static inline void ptep_copy(pte_t * to, pte_t * from)
{
	*to = *from;
}

#if 0
static inline void ptep_unset_perm(pte_t * ptep, pte_perm_t perm)
{
	*ptep &= ~perm;
}
#endif

static inline int ptep_accessed(pte_t * ptep)
{
	return (*ptep & PTE_A);
}

static inline int ptep_dirty(pte_t * ptep)
{
	return (*ptep & PTE_D);
}

static inline void ptep_set_accessed(pte_t * ptep)
{
//      *ptep |= PTE_A;
}

static inline void ptep_set_dirty(pte_t * ptep)
{
//      *ptep |= PTE_D;
}

static inline void ptep_unset_accessed(pte_t * ptep)
{
//      *ptep &= ~PTE_A;
}

static inline void ptep_unset_dirty(pte_t * ptep)
{
//      *ptep &= ~PTE_D;
}

#if 0
static inline pte_perm_t _arm_perm2ucore(pte_t * ptep)
{
	pte_perm_t p = 0;
	if (ptep_invalid(ptep))
		return p;
	if (ptep_present(ptep))
		p |= PTE_P;
	switch (*ptep & PTEX_PROTECT_MASK) {
	case PTEX_R:		//supervisor ro
		break;
	case PTEX_W:
		p |= PTE_W;
		break;
	case PTEX_U:
		p |= PTE_U;
		break;
	case PTEX_UW:
		p |= PTE_U | PTE_W;
		break;
	}
	return p;
}
#endif

#endif /* !__ARCH_X86_INCLUDE_PG_MAP_H__ */
