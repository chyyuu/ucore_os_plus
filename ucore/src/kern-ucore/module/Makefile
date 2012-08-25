obj-y := dde_main.o module_exporter.o module_loader.o

dirs-y := drivers fs kernel lib include ucore_glue
dirs-y += example

dirs-$(UCONFIG_ARM_BOARD_GOLDFISH) += goldfishdrv
dirs-$(UCONFIG_ARM_BOARD_PANDABOARD) += pandaboarddrv

dirs-$(UCONFIG_DDE_BLOCK_SUPPORT) += block		
