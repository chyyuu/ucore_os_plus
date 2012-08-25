SRCFILES	+= $(filter %.c %.S, $(wildcard arch/${ARCH}/*))
T_CC_ALL_FLAGS	+= -Iarch/${ARCH} -I../glue-kern/arch/${ARCH} -DBOOTLOADER_BASE=$(ARCH_ARM_BOOTLOADER_BASE)

include ${T_BASE}/mk/compbl.mk
include ${T_BASE}/mk/template.mk

all: ${T_OBJ}/bootloader-$(ARCH_ARM_BOARD)


LINK_FILE_IN	:= arch/${ARCH}/bootloader.ld.in
LINK_FILE     := $(T_OBJ)/bootloader-$(ARCH_ARM_BOARD).ld
SEDFLAGS	= s/TEXT_START/$(ARCH_ARM_BOOTLOADER_BASE)/


$(LINK_FILE): $(LINK_FILE_IN)
	@echo "creating linker script"
	@sed  "$(SEDFLAGS)" < $< > $@

${T_OBJ}/$(BOOTLOADER_FILENAME): ${OBJFILES} $(LINK_FILE)
	@echo LD $@
	@echo  $(ARCH_ARM_BOARD)
	${V}${LD} -T$(LINK_FILE) -o$@ ${OBJFILES}
