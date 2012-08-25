#
# Makefile for the kernel mmc core.
#

obj-y		+= core.o bus.o host.o \
				   mmc.o mmc_ops.o sd.o sd_ops.o \

obj-$(UCONFIG_DDE_MMC_HAVE_SDIO) +=  sdio.o sdio_ops.o sdio_bus.o \
				   sdio_cis.o sdio_io.o sdio_irq.o

TARGET_CFLAGS += -DKBUILD_MODNAME=\"mmc_core\"
