.PHONY: all

OFILELIST = 
TFILELIST = $(addprefix ${T_OBJ}/,${OFILELIST} bzImage.o)

include ${T_BASE}/mk/compk.mk

all: ${TFILELIST}

${T_OBJ}/%: %
	@echo CP $@
	${V}cp $< $@

${T_OBJ}/bzImage.o: bzImage
	@echo OBJCOPY $@
	${V}${OBJCOPY} -I binary -O elf64-x86-64 -B "i386:x86-64" $< $@

