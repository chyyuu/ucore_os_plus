SRCFILES	+= $(filter %.c %.S, $(wildcard arch/${ARCH}/*))
T_CC_ALL_FLAGS	+= -Iarch/${ARCH} -I../glue-kern/arch/${ARCH}

include ${T_BASE}/mk/compbl.mk
include ${T_BASE}/mk/template.mk

all: ${T_OBJ}/bootloader

${T_OBJ}/bootloader: ${OBJFILES}
	@echo LD $@
	${V}${LD} -Tarch/${ARCH}/bootloader.ld -o$@ ${OBJFILES}
