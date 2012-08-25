.PHONY: all mod-%

E_ENCODE ?= $(shell echo $(1) | sed -e 's!_!_1!g' -e 's!/!_2!g')
E_DECODE ?= $(shell echo $(1) | sed -e 's!_1!_!g' -e 's!_2!/!g')

# The order makes sense
ifeq (${BRANCH},ucore)
MODS := supervisor kern-ucore ht-sign ht-mksfs libs-user-ucore user-ucore bootloader
KERN := ucore
endif

ifeq (${BRANCH},river)
MODS := libs-sv supervisor kern-river ht-sign ht-mksfs libs-user-ucore user-ucore bootloader
KERN := river
endif

ifeq (${BRANCH},linux-dos-module)
MODS := linux-dos-module
else

ifeq (${HAS_DRIVER_OS},y)
MODS := prebuilt ${MODS}
mod-supervisor: mod-prebuilt
endif

endif

MODDIRS := $(addprefix mod-,${MODS})

all: ${MODDIRS} ${T_OBJ}/kernel.img ${T_OBJ}/swap.img ${T_OBJ}/sfs.img

mod-supervisor: mod-kern-ucore

mod-kern-river: mod-supervisor

mod-user-ucore: mod-ht-mksfs mod-libs-user-ucore

mod-bootloader: mod-ht-sign mod-kern-${KERN} mod-supervisor 

mod-%:
	@echo MAKE $* {
	${V}MOD="$*" ${MAKE} -C src/$* all
	@echo }

${T_OBJ}/kernel.img: ${T_OBJ}/bootsect ${T_OBJ}/kern-bin ${T_OBJ}/supervisor
	@echo MAKE $@
	${V}dd if=/dev/zero of=$@ count=10000
	${V}dd if=${T_OBJ}/bootsect of=$@ conv=notrunc
	${V}dd if=${T_OBJ}/kern-bin of=$@ seek=1 conv=notrunc
	${V}dd if=${T_OBJ}/supervisor of=$@ seek=$(shell echo $(shell cat ${T_OBJ}/kern-sect_size) + 1 | bc) conv=notrunc

${T_OBJ}/swap.img:
	@echo MAKE $@
	${V}dd if=/dev/zero of=$@ bs=1M count=128

${T_OBJ}/sfs.img: ${T_OBJ}/user-sfs-timestamp
	@echo MAKE $@
	${V}dd if=/dev/zero of=$@ bs=1M count=256
	${V}${T_OBJ}/tools-mksfs $@ ${T_OBJ}/user-sfs
