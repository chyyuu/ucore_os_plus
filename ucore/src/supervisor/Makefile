.PHONY: all

SRCFILES	:= $(shell find . '(' '!' -regex '.*/_.*' ')' -and '(' -iname "*.c" -or -iname "*.S" ')' -and -not -regex '\./arch.*' | sed -e 's!\./!!g')
T_CC_FLAGS	:= -D__SUPERVISOR__ -I../glue-kern/arch/${ARCH} -Idebug -Idriver -Iinit -Ilibs -Imm -Itrap -Isync -Imp -Icontext

include arch/$(ARCH)/supervisor.mk
