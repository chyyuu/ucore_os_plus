.PHONY: all clean

BE_ARCH	:= or32
ifneq (,$(findstring $(ARCH),$(BE_ARCH)))
OBJS := mksfs_be.c
else
OBJS := mksfs.c
endif
CFLAGS := -Wall -O2 -D_FILE_OFFSET_BITS=64

all: mksfs

mksfs: $(OBJS)
	$(CC) $(CFLAGS) -o $(OBJPATH_ROOT)/$@ $+
