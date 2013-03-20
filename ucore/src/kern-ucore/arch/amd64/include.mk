ARCH_INLUCDES:= debug driver include libs mm process sync trap syscall glue-ucore glue-ucore/libs module driver/acpica/source/include
ARCH_CFLAGS := -m64 -mcmodel=large -D__UCORE
ARCH_LDFLAGS := -melf_x86_64
