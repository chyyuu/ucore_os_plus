ARCH_CFLAGS := -DARCH_X86 -m32
ARCH_LDFLAGS := -melf_i386
ARCH_OBJS := clone.o syscall.o
ARCH_INITCODE_OBJ := initcode.o
