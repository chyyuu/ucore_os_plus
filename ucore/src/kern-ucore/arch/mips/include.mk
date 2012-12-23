ARCH_INLUCDES:=debug driver init mm libs process sync syscall trap glue-ucore 
ARCH_CFLAGS := -mips1 -DMACH_QEMU -g  -EL -G0 -fno-delayed-branch -Wall -O0
ARCH_LDFLAGS := -n -G 0 -static -EL

