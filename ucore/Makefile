
#--------------------------------------------------------------
# Just run 'make menuconfig', configure stuff, then run 'make'.
# You shouldn't need to mess with anything beyond this point...
#--------------------------------------------------------------
TOPDIR=$(shell pwd)
Q :=@


KTREE = $(TOPDIR)/src/kern-ucore
OBJPATH_ROOT := $(TOPDIR)/obj
export TOPDIR KTREE OBJPATH_ROOT

CONFIG = package/config

CONFIG_SHELL:=$(shell if [ -x "$$BASH" ]; then echo $$BASH; \
	else if [ -x /bin/bash ]; then echo /bin/bash; \
	else echo sh; fi; fi)

MAKEFLAGS += -rR --no-print-directory

#### CROSS COMPILE HERE ####
ARCH ?= arm
CROSS_COMPILE ?= arm-linux-

CONFIG_DIR := config
KCONFIG_AUTOCONFIG=config/auto.conf
KCONFIG_AUTOHEADER=config/autoconf.h
CONFIG_CONFIG_IN = $(KTREE)/arch/$(ARCH)/Kconfig
CONFIG_DEFCONFIG = .defconfig

export CONFIG_SHELL quiet Q KBUILD_VERBOSE
export ARCH CROSS_COMPILE
export KCONFIG_AUTOHEADER KCONFIG_AUTOCONFIG


TARGET_CC := $(CROSS_COMPILE)gcc
TARGET_LD := $(CROSS_COMPILE)ld
TARGET_AR := $(CROSS_COMPILE)ar
TARGET_STRIP := $(CROSS_COMPILE)strip
TARGET_OBJCOPY := $(CROSS_COMPILE)objcopy

export TARGET_CC TARGET_LD TARGET_AR TARGET_LD


ifndef HOSTAR
HOSTAR:=ar
endif
ifndef HOSTAS
HOSTAS:=as
endif
ifndef HOSTCC
HOSTCC:=gcc
else
endif
ifndef HOSTCXX
HOSTCXX:=g++
endif
ifndef HOSTLD
HOSTLD:=ld
endif
ifndef HOSTLN
HOSTLN:=ln
endif
HOSTAR:=$(shell $(CONFIG_SHELL) -c "which $(HOSTAR)" || type -p $(HOSTAR) || echo ar)
HOSTAS:=$(shell $(CONFIG_SHELL) -c "which $(HOSTAS)" || type -p $(HOSTAS) || echo as)
HOSTCC:=$(shell $(CONFIG_SHELL) -c "which $(HOSTCC)" || type -p $(HOSTCC) || echo gcc)
HOSTCXX:=$(shell $(CONFIG_SHELL) -c "which $(HOSTCXX)" || type -p $(HOSTCXX) || echo g++)
HOSTLD:=$(shell $(CONFIG_SHELL) -c "which $(HOSTLD)" || type -p $(HOSTLD) || echo ld)
HOSTLN:=$(shell $(CONFIG_SHELL) -c "which $(HOSTLN)" || type -p $(HOSTLN) || echo ln)
ifndef CFLAGS_FOR_BUILD
CFLAGS_FOR_BUILD:=-g -O2
endif
export HOSTAR HOSTAS HOSTCC HOSTCXX HOSTLD

HOSTCFLAGS=$(CFLAGS_FOR_BUILD)
export HOSTCFLAGS

PHONY+=menuconfig clean all

all: kernel

$(CONFIG)/mconf:
	$(MAKE) CC="$(HOSTCC)" -C $(CONFIG) conf mconf
	-@if [ ! -f .config ]; then \
		cp $(CONFIG_DEFCONFIG) .config; \
	fi

menuconfig: $(CONFIG_DIR) $(CONFIG)/mconf
	rm -f $(KCONFIG_AUTOCONFIG) $(KCONFIG_AUTOHEADER)
	@if ! $(CONFIG)/mconf $(CONFIG_CONFIG_IN); then \
		test -f .config.cmd || rm -f .config; \
	fi


PHONY += kernel userlib userapp

kernel: $(OBJPATH_ROOT) $(KCONFIG_AUTOHEADER) $(KCONFIG_AUTOCONFIG)
	$(Q)$(MAKE)  -C $(KTREE) -f $(KTREE)/Makefile.build

userlib: $(OBJPATH_ROOT) $(KCONFIG_AUTOCONFIG)
	$(Q)$(MAKE) -f $(TOPDIR)/src/libs-user-ucore/Makefile -C $(TOPDIR)/src/libs-user-ucore  all

userapp: $(OBJPATH_ROOT) $(KCONFIG_AUTOCONFIG)
	$(Q)$(MAKE) -f $(TOPDIR)/src/user-ucore/Makefile -C $(TOPDIR)/src/user-ucore  all

## TOOLS 

-include $(KCONFIG_AUTOCONFIG)

ifdef UCONFIG_HAVE_SFS
TOOLS_MKSFS_DIR := $(TOPDIR)/src/ht-mksfs
TOOLS_MKSFS := $(TOOLS_MKSFS_DIR)/mksfs
$(TOOLS_MKSFS): $(OBJPATH_ROOT)
	$(Q)$(MAKE) CC=$(HOSTCC) -f $(TOOLS_MKSFS_DIR)/Makefile -C $(TOOLS_MKSFS_DIR) all

## image
SFSIMG_FILE := sfs.img
sfsimg: $(SFSIMG_FILE)

$(SFSIMG_FILE): $(OBJPATH_ROOT) $(TOOLS_MKSFS) userlib userapp FORCE
	@echo Making $@
	mkdir .tmpsfs
	cp -r $(TOPDIR)/src/user-ucore/bin .tmpsfs/
	-cp -r $(TOPDIR)/src/user-ucore/_initial/* .tmpsfs/
	rm -f $@
	dd if=/dev/zero of=$@ bs=1M count=$(UCONFIG_SFS_IMAGE_SIZE)
	$(TOOLS_MKSFS) $@ .tmpsfs
	rm -rf .tmpsfs

endif

$(OBJPATH_ROOT):
	-mkdir $@

$(CONFIG_DIR):
	-mkdir config

#	$(Q)rm -f $(KCONFIG_AUTOCONFIG) $(KCONFIG_AUTOHEADER)
clean:
	@echo CLEAN ALL
	$(Q)rm -f  .config.old .config.cmd .tmpconfig.h
	$(Q)rm -f $(SFSIMG_FILE)
	$(Q)rm -rf $(OBJPATH_ROOT)
	-$(Q)$(MAKE) -C $(CONFIG) clean
	$(Q)$(MAKE) -C $(KTREE) -f Makefile.build clean
	$(Q)$(MAKE) -f $(TOPDIR)/src/libs-user-ucore/Makefile -C $(TOPDIR)/src/libs-user-ucore  clean
	$(Q)$(MAKE) -f $(TOPDIR)/src/user-ucore/Makefile -C $(TOPDIR)/src/user-ucore  clean

FORCE:

PHONY += FORCE

.PHONY: $(PHONY)
