ARCH_INLUCDES:= debug driver include libs mm process sync trap syscall glue-ucore glue-ucore/libs module
ARCH_CFLAGS := -m64 -mcmodel=large
ARCH_LDFLAGS := -melf_x86_64
