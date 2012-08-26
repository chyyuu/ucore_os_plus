.PHONY: all

MOD    := bootloader

T_BASE := $(TOPDIR)
T_OBJ  := $(OBJPATH_ROOT)/bootloader

CC     := $(TARGET_CC)
LD     := $(TARGET_LD)
OBJDUMP:= $(TARGET_OBJDUMP)
OBJCOPY:= $(TARGET_OBJCOPY)
STRIP  := $(TARGET_STRIP)

HT_SIGN:= $(OBJPATH_ROOT)/sign
HT_SIGN_DIR := $(TOPDIR)/src/ht-sign

V      ?= @

SRCFILES   := $(shell find . '(' '!' -regex '.*/_.*' ')' -and '(' -iname "*.c" -or -iname "*.S" ')' -and -not -regex '\./arch.*' | sed -e 's!\./!!g')
T_CC_ALL_FLAGS := -Os -I.

include arch/${ARCH}/bootloader.mk

$(T_OBJ):
	@mkdir $(T_OBJ)

$(HT_SIGN):
	$(Q)$(MAKE) HOST_CC_PREFIX= -C $(HT_SIGN_DIR) -f $(HT_SIGN_DIR)/Makefile all
