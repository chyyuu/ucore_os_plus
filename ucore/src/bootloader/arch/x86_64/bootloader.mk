SRCFILES	+= $(filter %.c %.S, $(wildcard arch/${ARCH}/*))
T_CC_ALL_FLAGS	+= -Iarch/${ARCH} -DKERN_START_SECT=$(shell echo $(shell cat ${T_OBJ}/kern-sect_size) + 1 | bc)  -D__ARCH_X86_64__

include ${T_BASE}/mk/compbl.mk
include ${T_BASE}/mk/template.mk

all: ${T_OBJ}/bootsect

${T_OBJ}/bootsect: ${T_OBJ}/bootloader
	@echo OBJCOPY $@
	${V}${OBJCOPY} -S -O binary $^ $@.original
	@${T_OBJ}/tools-sign $@.original $@

${T_OBJ}/bootloader: ${OBJFILES}
	@echo LD $@
	${V}${LD} -N -e start -Tarch/${ARCH}/bootloader.ld -o$@ ${OBJFILES}
	${V}${STRIP} -g -R .eh_frame $@
