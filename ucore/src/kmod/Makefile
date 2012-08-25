.PHONY: all

KERNEL_INC := -I$(T_BASE)/src/kern-ucore/module/include -include linux/autoconf.h -I$(T_BASE)/src/kern-ucore/glue-ucore/libs/ -I$(T_BASE)/src/kern-ucore/module/

include ${T_BASE}/mk/compk.mk
include ${T_BASE}/mk/template.mk

hello.ko: hello.c hello.mod.c
	$(CC) -c -D__KERNEL__ -DMODULE $(KERNEL_INC) $^
	$(CC) -c -D__KERNEL__ -DMODULE $(KERNEL_INC) hello.mod.c
	$(LD) -r -o $@ hello.o hello.mod.o

all: hello.ko

