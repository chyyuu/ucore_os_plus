.PHONY: all clean

obj-m     := dosm.o
dosm-objs := dosm-init.o dosm-packet.o

LINUX_BUILD ?= ${T_BASE}/linux-src-link

all: ${T_OBJ}/dosm.ko

${T_OBJ}/dosm.ko: dosm.ko
	${V}cp $< $@

dosm.ko:
	@echo MAKE MODULE linux-dos-module
	${V}ARCH=x86_64 make -C ${LINUX_BUILD} M=$(shell pwd) modules

clean:
	@echo CLEAN MODULE linux-dos-module
	${V}ARCH=x86_64 make -C ${LINUX_BUILD} M=$(shell pwd) clean
