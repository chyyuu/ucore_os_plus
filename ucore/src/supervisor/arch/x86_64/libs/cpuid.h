#ifndef __CPUID__
#define __CPUID__

static inline void
cpuid(uint32_t info, uint32_t * eaxp, uint32_t * ebxp, uint32_t * ecxp,
      uint32_t * edxp)
{
	uint32_t eax, ebx, ecx, edx;
	asm volatile ("cpuid":"=a" (eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
		      :"a"(info));
	if (eaxp)
		*eaxp = eax;
	if (ebxp)
		*ebxp = ebx;
	if (ecxp)
		*ecxp = ecx;
	if (edxp)
		*edxp = edx;
}

#endif
