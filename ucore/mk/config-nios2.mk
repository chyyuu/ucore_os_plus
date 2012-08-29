export HOST_CC_PREFIX	?=
export TARGET_CC_PREFIX	?= nios2-elf-
export TARGET_CC_FLAGS_COMMON	?=	-fno-builtin  -nostdinc -g   \
									-EL -mhw-div -mhw-mul -mno-hw-mulx -DSYSTEM_BUS_WIDTH=32 \
									-MP -MMD -pipe -DALT_NO_INSTRUCTION_EMULATION -Wno-traditional -G 0 \
									-fomit-frame-pointer
export TARGET_CC_FLAGS_BL		?=
export TARGET_CC_FLAGS_KERNEL	?=
export TARGET_CC_FLAGS_SV		?=
export TARGET_CC_FLAGS_USER		?=
export TARGET_LD_FLAGS			?=	-m    nios2elf     -nostdlib
