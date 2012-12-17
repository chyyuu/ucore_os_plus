ARCH_CFLAGS := -EL -mhw-div -mhw-mul -mno-hw-mulx -DSYSTEM_BUS_WIDTH=32 \
               -pipe -DALT_NO_INSTRUCTION_EMULATION -Wno-traditional -G 0 \
               -fomit-frame-pointer -Wall -O2
ARCH_LDFLAGS := -m    nios2elf
ARCH_OBJS := clone.o syscall.o libgcc.o
ARCH_INITCODE_OBJ := initcode.o
