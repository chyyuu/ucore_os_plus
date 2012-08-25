SRCFILES	+= $(filter %.c %.S, $(wildcard arch/${ARCH}/*))
T_CC_ALL_FLAGS	+= -Iarch/${ARCH} -D__ARCH_I386__

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
