ARCH_INLUCDES := . debug driver include libs mm numa process sync trap syscall kmodule driver/acpica/source/include
ARCH_CFLAGS := -m64 -mcmodel=large -D__UCORE
ARCH_LDFLAGS := -melf_x86_64
