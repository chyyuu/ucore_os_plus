ARCH_CFLAGS := -mips1 -g  -EL -G0 -fno-delayed-branch -Wall -O0 -DMACH_QEMU
ARCH_LDFLAGS := 
ARCH_OBJS := syscall.o initcode.o intr.o clone.o udivmod.o udivmodsi4.o divmod.o
ARCH_INITCODE_OBJ := initcode.o
