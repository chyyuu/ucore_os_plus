SRCFILES	+= $(filter %.c %.S, $(wildcard arch/${ARCH}/*))
T_CC_FLAGS	+= -Iarch/${ARCH} -DNO_LOCK

include ${T_BASE}/mk/compu.mk
include ${T_BASE}/mk/template.mk

all: ${T_OBJ}/ulib.a

${T_OBJ}/ulib.a: ${OBJFILES}
	@echo AR $@
	${V}${TARGET_CC_PREFIX}ar -cr $@ $+
