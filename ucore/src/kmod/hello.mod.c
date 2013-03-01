#include <linux/module.h>
#include <linux/compiler.h>

struct module __this_module
    __attribute__ ((section(".gnu.linkonce.this_module"))) = {
	.name = "hello",.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	    .exit = cleanup_module,
#endif
.arch = MODULE_ARCH_INIT,};
