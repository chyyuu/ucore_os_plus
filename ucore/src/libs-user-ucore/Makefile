.PHONY: all clean

TARGET_CFLAGS := -I. -Icommon -Iarch/$(ARCH) -nostdinc -nostdlib -fno-builtin
obj-y := dir.o file.o malloc.o panic.o signal.o spipe.o \
				stdio.o string.o syscall.o thread.o ulib.o umain.o
obj-y += common/hash.o common/rand.o common/printfmt.o \
				common/string.o

include arch/$(ARCH)/include.mk

obj-y += $(addprefix arch/$(ARCH)/, $(ARCH_OBJS))
obj-initobj += $(addprefix arch/$(ARCH)/, $(ARCH_INITCODE_OBJ))
TARGET_CFLAGS += $(ARCH_CFLAGS)

all: $(obj-initobj) ulib.a

ulib.a: $(obj-y)
	$(TARGET_AR) -cr $@ $+

%.o: %.c
	$(TARGET_CC) $(TARGET_CFLAGS) -c -o $@ $<

%.o: %.S
	$(TARGET_CC) -D__ASSEMBLY__ $(TARGET_CFLAGS) -c -o $@ $<

clean:
	find . -name \*.o -exec rm -f {} \;
	rm -f ulib.a
