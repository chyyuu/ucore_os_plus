.PHONY: all clean

-include $(TOPDIR)/$(KCONFIG_AUTOCONFIG)

TARGET_CFLAGS:= 	-I../libs-user-ucore/ -I../libs-user-ucore/common -I../libs-user-ucore/arch/$(ARCH) -nostdinc -nostdlib -fno-builtin

#TARGET_CC_SYSTEM_LIB ?=  -L/opt/FriendlyARM/toolschain/4.4.3/lib/gcc/arm-none-linux-gnueabi/4.4.3/ 

USER_LIB_SRCDIR:= ../libs-user-ucore
USER_LIB := $(USER_LIB_SRCDIR)/ulib.a
BIN:=bin

USER_APPLIST:= pwd cat sh ls pgdir cp forktest #yield hello faultreadkernel faultread badarg waitkill pgdir exit sleep sigtest1

USER_APP_BINS:= $(addprefix $(BIN)/, $(USER_APPLIST))

include $(USER_LIB_SRCDIR)/arch/$(ARCH)/include.mk
INITCODE_OBJ := $(USER_LIB_SRCDIR)/arch/$(ARCH)/$(ARCH_INITCODE_OBJ)

all:  $(BIN) $(USER_LIB) $(USER_APP_BINS)

#user applications
define make-user-app
$(BIN)/$1: $(addsuffix .o,$1) $(INITCODE_OBJ) $(USER_LIB)
	@echo LINK $$@
	$(TARGET_LD)  -static -T $(USER_LIB_SRCDIR)/arch/$(ARCH)/user.ld  $(addsuffix .o,$1) $(INITCODE_OBJ) $(USER_LIB) -o $$@
endef

$(foreach bdir,$(USER_APPLIST),$(eval $(call make-user-app,$(bdir))))

%.o: %.c
	$(TARGET_CC) $(TARGET_CFLAGS) -c -o $@ $<

$(BIN):
	-mkdir $(BIN)

clean:
	find . -name \*.o -exec rm -f {} \;
	rm -rf $(BIN)
