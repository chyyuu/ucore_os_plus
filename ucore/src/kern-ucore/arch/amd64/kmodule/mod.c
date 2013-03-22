#include <elf.h>
#include <mod.h>

int apply_relocate(struct secthdr *sechdrs,
		   const char *strtab,
		   unsigned int symindex,
		   unsigned int relsec, struct module *mod)
{
	unsigned int i;
	struct reloc_a_s *rel = (void *)sechdrs[relsec].sh_addr;
	struct symtab_s *sym;
	void *loc;
	uint64_t val;

	kprintf("Applying relocate section %u to %u\n", relsec,
		sechdrs[relsec].sh_info);
	for (i = 0; i < sechdrs[relsec].sh_size / sizeof(*rel); i++) {
		/* This is where to make the change */
		loc = (void *)sechdrs[sechdrs[relsec].sh_info].sh_addr
		    + rel[i].r_offset;
		/* This is the symbol it is referring to.  Note that all
		   undefined symbols have been resolved.  */
		sym = (struct symtab_s *)sechdrs[symindex].sh_addr
		    + GET_RELOC_SYM(rel[i].r_info);
		val = sym->st_value + rel[i].r_addend;
		switch (GET_RELOC_TYPE(rel[i].r_info)) {
		case R_X86_64_NONE:
			break;
		case R_X86_64_64:
			/* Add the value, subtract its postition */
			*(uint64_t *) loc = val;
			break;
		case R_X86_64_32:
			*(uint32_t *) loc = val;
			if (val != *(uint32_t *) loc)
				goto overflow;
			break;
		case R_X86_64_32S:
			*(int32_t *) loc = val;
			if ((int64_t) val != *(int32_t *) loc)
				goto overflow;
			break;
		case R_X86_64_PC32:
			val -= (uint64_t) loc;
			*(uint32_t *) loc = val;
			break;
		default:
			kprintf
			    ("apply_relocate_add: module %s: Unknown relocation: %llu\n",
			     mod->name, GET_RELOC_TYPE(rel[i].r_info));
			return -1;
		}
	}
	return 0;

overflow:
	kprintf("apply_relocate_add: overflow in relocation type %d val %LX\n",
		(int)GET_RELOC_TYPE(rel[i].r_info), val);
	kprintf
	    ("apply_relocate_add: %s likely not compiled with -mcmodel=kernel\n",
	     mod->name);
	return -1;

}

/* Apply the given add relocation to the (simplified) ELF.  Return
	-error or 0 */
int apply_relocate_add(struct secthdr *sechdrs,
		       const char *strtab,
		       unsigned int symindex,
		       unsigned int relsec, struct module *mod)
{
	kprintf("apply_relocate_add: module %s: ADD RELOCATION unsupported\n",
		mod->name);
	return -1;
}
