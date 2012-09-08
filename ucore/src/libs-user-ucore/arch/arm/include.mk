ARCH_CFLAGS := -DARCH_ARM
ARCH_OBJS := clone.o div64.o
ARCH_INITCODE_OBJ := initcode.o
USER_LIB += $(shell $(TARGET_CC) --print-file-name=libgcc.a)
