ARCH_CFLAGS := -DARCH_ARM
ARCH_OBJS := clone.o div64.o
ARCH_INITCODE_OBJ := initcode.o

# Avoid shell warning on null TARGET_CC which is possible when creating link to
# initial dir
ifdef TARGET_CC
USER_LIB += $(shell $(TARGET_CC) --print-file-name=libgcc.a)
endif
