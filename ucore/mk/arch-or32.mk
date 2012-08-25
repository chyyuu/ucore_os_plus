.PHONY: all mod-% run

E_ENCODE ?= $(shell echo $(1) | sed -e 's!_!_1!g' -e 's!/!_2!g')
E_DECODE ?= $(shell echo $(1) | sed -e 's!_1!_!g' -e 's!_2!/!g')

MODS := bootloader kern-ucore ht-sign ht-mksfs ht-mkram libs-user-ucore user-ucore

MODDIRS := $(addprefix mod-,${MODS})

SWAPIMG		:= ${T_OBJ}/swap.img
FSIMG		:= ${T_OBJ}/sfs.img
RAMIMG		:= ${T_OBJ}/ram.img

all: ${MODDIRS} ${SWAPIMG} ${FSIMG} ${RAMIMG}

mod-user-ucore: mod-ht-mksfs mod-libs-user-ucore

mod-%:
	@echo MAKE $* {
	${V}MOD="$*" ${MAKE} -C src/$* all
	@echo }

${SWAPIMG}:
	@echo MAKE $@
	${V}dd if=/dev/zero of=$@ bs=1M count=4

${FSIMG}: ${T_OBJ}/user-sfs-timestamp
	@echo MAKE $@
	${V}dd if=/dev/zero of=$@ bs=1M count=8
	${V}${T_OBJ}/tools-mksfs $@ ${T_OBJ}/user-sfs

${RAMIMG}: ${T_OBJ}/kernel ${FSIMG} ${T_OBJ}/tools-mkram
	@echo MAKE $@
	${V}${T_OBJ}/tools-mkram ${T_OBJ}/kernel ${RAMIMG} -f ${FSIMG}
