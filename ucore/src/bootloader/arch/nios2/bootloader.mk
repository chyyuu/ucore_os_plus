BOOTSECT   := $(T_OBJ)/boot_loader_cfi.srec

all: $(BOOTSECT)

$(BOOTSECT): arch/${ARCH}/boot_loader_cfi.srec
	${V}mkdir $(T_OBJ)
	${V}cp arch/${ARCH}/boot_loader_cfi.srec $(BOOTSECT)


