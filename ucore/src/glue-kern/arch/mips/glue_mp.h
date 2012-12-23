#ifndef __GLUE_MP_H__
#define __GLUE_MP_H__
#define LAPIC_COUNT 1

#define PLS

#define pls_read(var) pls_##var

#define pls_get_ptr(var) &pls_##var

#define pls_write(var, value)											\
	do {																\
		pls_##var = value;                                              \
	} while (0)

#endif /* !__GLUE_MP_H__ */
