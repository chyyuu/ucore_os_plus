.PHONY: all mod-% run

E_ENCODE ?= $(shell echo $(1) | sed -e 's!_!_1!g' -e 's!/!_2!g')
E_DECODE ?= $(shell echo $(1) | sed -e 's!_1!_!g' -e 's!_2!/!g')

MODS := bootloader kern-ucore ht-mksfs libs-user-ucore user-ucore

MODDIRS := $(addprefix mod-,${MODS})

SWAPIMG		:= ${T_OBJ}/swap.img
FSIMG		:= ${T_OBJ}/sfs.img

all: ${MODDIRS} ${SWAPIMG} ${FSIMG}

mod-user-ucore: mod-ht-mksfs mod-libs-user-ucore

mod-%:
	@echo MAKE $* {
	${V}MOD="$*" ${MAKE} -C src/$* all
	@echo }

${T_OBJ}/swap.img:
	@echo MAKE $@
	${V}dd if=/dev/zero of=$@ bs=1M count=128

${T_OBJ}/sfs.img: ${T_OBJ}/user-sfs-timestamp
	@echo MAKE $@
	${V}dd if=/dev/zero of=$@ bs=1M count=128
	${V}${T_OBJ}/tools-mksfs $@ ${T_OBJ}/user-sfs
