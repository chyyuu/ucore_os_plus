#ifndef __ARCH_X86_INCLUDE_PG_MAP_H__
#define __ARCH_X86_INCLUDE_PG_MAP_H__

typedef pte_t pte_perm_t;

static inline void ptep_map(pte_t * ptep, uintptr_t pa)
{
	*ptep = pa | PTE_P | PTE_SPR_R | PTE_E;
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
	return (*ptep & PTE_P);
}

static inline int ptep_s_read(pte_t * ptep)
{
	return (*ptep & PTE_SPR_R);
}

static inline int ptep_s_write(pte_t * ptep)
{
	return (*ptep & PTE_SPR_W);
}

static inline int ptep_s_exec(pte_t * ptep)
{
	return (*ptep & PTE_E);
}

static inline int ptep_u_read(pte_t * ptep)
{
	return (*ptep & PTE_USER_R);
}

static inline int ptep_u_write(pte_t * ptep)
{
	return (*ptep & PTE_USER_W);
}

static inline int ptep_u_exec(pte_t * ptep)
{
	return (*ptep & PTE_E);
}

static inline void ptep_set_s_read(pte_t * ptep)
{
	*ptep |= PTE_SPR_R;
}

static inline void ptep_set_s_write(pte_t * ptep)
{
	*ptep |= PTE_SPR_W | PTE_SPR_R;
}

static inline void ptep_set_s_exec(pte_t * ptep)
{
	*ptep |= PTE_E;
}

static inline void ptep_set_u_read(pte_t * ptep)
{
	*ptep |= (PTE_USER_R | PTE_SPR_R);
}

static inline void ptep_set_u_write(pte_t * ptep)
{
	*ptep |= (PTE_USER_R | PTE_USER_W | PTE_SPR_R | PTE_SPR_W);
}

static inline void ptep_set_u_exec(pte_t * ptep)
{
	*ptep |= PTE_E;
}

static inline void ptep_unset_s_read(pte_t * ptep)
{
	*ptep &= ~(PTE_SPR_R | PTE_USER_R);
}

static inline void ptep_unset_s_write(pte_t * ptep)
{
	*ptep &= ~(PTE_SPR_W | PTE_USER_W);
}

static inline void ptep_unset_s_exec(pte_t * ptep)
{
	*ptep &= ~PTE_E;
}

static inline void ptep_unset_u_read(pte_t * ptep)
{
	*ptep &= ~PTE_USER_R;
}

static inline void ptep_unset_u_write(pte_t * ptep)
{
	*ptep &= ~PTE_USER_W;
}

static inline void ptep_unset_u_exec(pte_t * ptep)
{
	*ptep &= ~PTE_E;
}

static inline pte_perm_t ptep_get_perm(pte_t * ptep, pte_perm_t perm)
{
	return (*ptep) & perm;
}

static inline void ptep_set_perm(pte_t * ptep, pte_perm_t perm)
{
	*ptep |= perm;
}

static inline void ptep_copy(pte_t * to, pte_t * from)
{
	*to = *from;
}

static inline void ptep_unset_perm(pte_t * ptep, pte_perm_t perm)
{
	*ptep &= ~perm;
}

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
	*ptep |= PTE_A;
}

static inline void ptep_set_dirty(pte_t * ptep)
{
	*ptep |= PTE_D;
}

static inline void ptep_unset_accessed(pte_t * ptep)
{
	*ptep &= ~PTE_A;
}

static inline void ptep_unset_dirty(pte_t * ptep)
{
	*ptep &= ~PTE_D;
}

#endif /* !__ARCH_X86_INCLUDE_PG_MAP_H__ */
