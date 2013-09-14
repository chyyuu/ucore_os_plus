#!/bin/bash

BUILD_DIR=obj
COMMAND_LINE_MODE=
DEBUG_MODE=
TEST_MODE=
TEST_POSTFIX=
GDB_PORT=1234

while getopts "chgd:t:" opt; do
    case $opt in
        c)
            COMMAND_LINE_MODE=y
            ;;
        d)
            BUILD_DIR=$OPTARG
            ;;
        h)
            echo "Usage $0 [options]"
            echo "Options:"
            echo "  -c                             run in command prompt w/o graphic"
            echo "  -d <directory>                 uCore build directory"
            echo "                                 default to obj/"
            echo "  -t                             run in background for test"
            echo "                                 i.e. do not use stdio or open SDL window"
            echo "                                 and use images with the given postfix"
            echo ""
            echo "Supported architectures: i386, arm(goldfishv7), amd64"
            echo ""
            echo "Report bugs to https://github.com/chyyuu/ucore_plus/issues"
            exit 0
            ;;
        g)
            DEBUG_MODE=y
            ;;
        t)
            TEST_MODE=y
            TEST_POSTFIX=.$OPTARG
            GDB_PORT=$[ $OPTARG % 10000 + 40000 ]
            ;;
        ?)
            exit 1
            ;;
    esac
done

# If we are invoked w/o GUI, fall back to command line mode
if [[ -z $DISPLAY ]]; then
    COMMAND_LINE_MODE=y
fi

if [[ ! -d $BUILD_DIR ]]; then
    echo $BUILD_DIR does not exist or is not a directory.
    echo Use -d to specify your custom build directory.
    exit 1
fi

if [[ ! -e $BUILD_DIR/config/auto.conf ]]; then
    echo Is $BUILD_DIR properly configured?
    exit 1
fi

source $BUILD_DIR/config/auto.conf

case $UCONFIG_ARCH in
    arm)
        if [[ $UCONFIG_ARM_BOARD_GOLDFISH = y ]]; then
            if [[ $AVD = "" ]]; then
                AVD=`android list avd | grep "Name" -m 1 | sed "s/[ \t]*Name:[ \t]*//"`
                echo AVD not specified. Fetch the first avd from \"android list avd\" and get \"$AVD\"
            fi
            if [[ $TEST_MODE = y ]]; then
                QEMU_OPT="-S -serial file:$BUILD_DIR/serial.log$TEST_POSTFIX -gdb tcp::$GDB_PORT"
            else
                QEMU_OPT="-serial stdio"
                if [[ $DEBUG_MODE = y ]]; then
                    QEMU_OPT+=" -s -S"
                fi
            fi
            exec emulator-arm -avd $AVD -kernel $BUILD_DIR/kernel.img$TEST_POSTFIX -no-window -qemu $QEMU_OPT
        else
            echo What kind of board for ARM is this?
            echo Consider supporting your board in uCore_run
            exit 1
        fi
        ;;
    i386)
        QEMU=qemu
        which qemu-system-i386 > /dev/null 2>&1
        if [ $? -eq 0 ]; then
            QEMU=qemu-system-i386
        fi
        EXTRA_OPT="-serial file:$BUILD_DIR/serial.log$TEST_POSTFIX"
        if [[ $TEST_MODE = y ]]; then
            EXTRA_OPT+=" -S -display none -gdb tcp::$GDB_PORT"
        else
            if [[ $COMMAND_LINE_MODE = y ]]; then
                echo -e "\033[0;36;1mNote: Qemu is running under no graphic mode. Press 'Ctrl-a x' to exit\033[0m"
                EXTRA_OPT="-nographic"
            else
                EXTRA_OPT+=" -monitor stdio -serial file:$BUILD_DIR/serial.log$TEST_POSTFIX"
            fi
            if [[ $DEBUG_MODE = y ]]; then
                EXTRA_OPT+=" -s -S"
            fi
        fi
        exec $QEMU -m 128 \
            -hda $BUILD_DIR/kernel.img$TEST_POSTFIX \
            -drive file=$BUILD_DIR/sfs.img$TEST_POSTFIX,media=disk,cache=writeback,index=2 \
            $EXTRA_OPT
        ;;
    amd64)
        QEMU=qemu
        which qemu-system-x86_64 > /dev/null 2>&1
        if [ $? -eq 0 ]; then
            QEMU=qemu-system-x86_64
        fi
        EXTRA_OPT="-serial file:$BUILD_DIR/serial.log$TEST_POSTFIX"
        if [[ $TEST_MODE = y ]]; then
            EXTRA_OPT+=" -S -display none -gdb tcp::$GDB_PORT "
        else
            if [[ $COMMAND_LINE_MODE = y ]]; then
                echo -e "\033[0;36;1mNote: Qemu is running under no graphic mode. Press 'Ctrl-a x' to exit\033[0m"
                EXTRA_OPT="-nographic"
            else
                EXTRA_OPT+=" -monitor stdio -serial file:$BUILD_DIR/serial.log$TEST_POSTFIX"
            fi
            if [[ $DEBUG_MODE = y ]]; then
                EXTRA_OPT+=" -s -S"
            fi
        fi
        exec $QEMU -m 512 \
            -hda $BUILD_DIR/kernel.img$TEST_POSTFIX \
            -drive file=$BUILD_DIR/sfs.img$TEST_POSTFIX,media=disk,cache=writeback,index=2 \
            $EXTRA_OPT
        ;;
esac
