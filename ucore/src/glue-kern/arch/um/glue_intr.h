#ifndef __GLUE_INTR_H__
#define __GLUE_INTR_H__

#include <intr.h>
#include <sync.h>

/**
 * Umucore doesn't have the so-called 'trapframe'.
 *     This is hacked only because 'do_fork' needs it...
 */
struct trapframe {
	int (*fn) (void *);
	void *arg;
};

#define local_intr_enable_hw  intr_enable()
#define local_intr_disable_hw intr_disable()

#define local_intr_save_hw(x)      local_intr_save(x)
#define local_intr_restore_hw(x)   local_intr_restore(x)

#endif /* !__GLUE_INTR_H__ */
