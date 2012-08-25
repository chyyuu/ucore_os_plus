.PHONY: all mod-% run

E_ENCODE ?= $(shell echo $(1) | sed -e 's!_!_1!g' -e 's!/!_2!g')
E_DECODE ?= $(shell echo $(1) | sed -e 's!_1!_!g' -e 's!_2!/!g')

MODS := bootloader kern-ucore kmod# libs-user-ucore user-ucore

MODDIRS := $(addprefix mod-,${MODS})

SWAPIMG		:= ${T_OBJ}/swap.img
FSIMG		:= ${T_OBJ}/sfs.img
RAMIMG		:= ${T_OBJ}/ram.img
KERNEL_IMG := $(T_OBJ)/kernel-$(ARCH_ARM_BOARD).img

#export DATA_FILE ?= $(T_OBJ)/../misc/rootfs.img
export DATA_FILE ?= $(FSIMG)

all: mod-user-ucore $(FSIMG) ${MODDIRS} ${KERNEL_IMG} #${SWAPIMG} ${RAMIMG}

mod-user-ucore: mod-ht-mksfs mod-libs-user-ucore

mod-%:
	@echo MAKE $* {
	${V}MOD="$*" ${MAKE} -C src/$* all
	@echo }


$(KERNEL_IMG): $(T_OBJ)/$(KERNEL_INITRD_FILENAME) $(T_OBJ)/$(BOOTLOADER_FILENAME)
	$(OBJCOPY) -S -O binary $(T_OBJ)/$(BOOTLOADER_FILENAME) $(T_OBJ)/$(BOOTLOADER_FILENAME).binary
	dd if=/dev/zero of=$@.noheader bs=16M count=1 
	dd if=$(T_OBJ)/$(BOOTLOADER_FILENAME).binary of=$@.noheader conv=notrunc
	dd if=$(T_OBJ)/$(KERNEL_INITRD_FILENAME) of=$@.noheader seek=8 conv=notrunc
	mkimage -A arm -O linux -T kernel -C none -a $(ARCH_ARM_BOOTLOADER_BASE) -e $(ARCH_ARM_BOOTLOADER_BASE) -n "ucore-arm" -d $@.noheader $@

${SWAPIMG}:
	@echo $@ NOT impl

${FSIMG}: mod-user-ucore
	@echo "Making FS IMG"
	dd if=/dev/zero of=$@ bs=1M count=48
	$(T_OBJ)/tools-mksfs $@ $(T_OBJ)/user-sfs

${RAMIMG}: ${T_OBJ}/$(KERNEL_FILENAME)
	@echo $@ NOT impl


