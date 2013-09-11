ARCH_CFLAGS := -DARCH_AMD64
ARCH_LDFLAGS := -z max-page-size=0x1000  -melf_x86_64
ARCH_OBJS := clone.o syscall.o
ARCH_INITCODE_OBJ := initcode.o
