.PHONY: all

SRCFILES   := ${ARCH}/mkram.c
T_CC_FLAGS := -Wall -O2 -fno-strict-aliasing

include ${T_BASE}/mk/comph.mk
include ${T_BASE}/mk/template.mk

all: ${T_OBJ}/tools-mkram

${T_OBJ}/tools-mkram: ${OBJFILES}
	@echo LD $@
	${V}${CC} -o$@ $^
