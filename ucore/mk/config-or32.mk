export HOST_CC_PREFIX	?=
export TARGET_CC_PREFIX	?= or32-elf-
export TARGET_CC_FLAGS_COMMON	?= -fno-builtin -nostdinc -fno-stack-protector -mhard-mul -D__BIG_ENDIAN__
export TARGET_CC_FLAGS_BL		?=
export TARGET_CC_FLAGS_KERNEL	?=
export TARGET_CC_FLAGS_SV		?=
export TARGET_CC_FLAGS_USER		?=
export TARGET_LD_FLAGS			?= -nostdlib

TERMINAL	?= gnome-terminal
SIMULATOR	?= or32-elf-sim
RAMIMG		:= ${T_OBJ}/ram.img

run: all
	${V}${TERMINAL} -e "${SIMULATOR} -f misc/or32-sim.cfg ${RAMIMG}"

debug: all
	${V}${TERMINAL} -e "rlwrap ${SIMULATOR} -f misc/or32-sim.cfg ${RAMIMG} -i"
