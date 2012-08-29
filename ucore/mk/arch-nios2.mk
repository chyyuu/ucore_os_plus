.PHONY: all mod-%

E_ENCODE ?= $(shell echo $(1) | sed -e 's!_!_1!g' -e 's!/!_2!g')
E_DECODE ?= $(shell echo $(1) | sed -e 's!_1!_!g' -e 's!_2!/!g')

#MODS := bootloader kern-ucore ht-sign ht-mksfs libs-user-ucore 
MODS := kern-ucore ht-mksfs libs-user-ucore user-ucore
#MODS := kern-ucore
KERN := ucore

MODDIRS := $(addprefix mod-,${MODS})

#all: ${MODDIRS} ${T_OBJ}/kernel.img ${T_OBJ}/swap.img ${T_OBJ}/sfs.img
all: ${MODDIRS} ${T_OBJ}/kernel.elf ${T_OBJ}/sfs.img

mod-user-ucore: mod-ht-mksfs mod-libs-user-ucore

mod-bootloader: mod-ht-sign 

mod-%:
	@echo MAKE $* {
	${V}MOD="$*" ${MAKE} -C src/$* all
	@echo }

${T_OBJ}/kernel.elf: ${T_OBJ}/kern-bin
	@echo MAKE $@
	${V}mv ${T_OBJ}/kern-bin ${T_OBJ}/kernel.elf

${T_OBJ}/sfs.img: ${T_OBJ}/user-sfs-timestamp
	@echo MAKE $@
	${V}dd if=/dev/zero of=$@ bs=1M count=3
	${V}${T_OBJ}/tools-mksfs $@ ${T_OBJ}/user-sfs

