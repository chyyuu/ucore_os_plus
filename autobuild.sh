#!/bin/bash

pushd `dirname $0` > /dev/null
source autotest.config
cd ucore

# Build uCore for i386
make O=$BUILD_DIR_I386 ARCH=i386 defconfig
make O=$BUILD_DIR_I386 kernel
UCORE_TEST=xx make O=$BUILD_DIR_I386 sfsimg
make O=$BUILD_DIR_I386 swapimg

# Build uCore for arm on goldfishv7
# Disable DDE at present as it violates the checks after all user-mode processes quit
mkdir -p $BUILD_DIR_ARM
grep -v DDE src/kern-ucore/arch/arm/configs/goldfishv7_defconfig > $BUILD_DIR_ARM/.defconfig
echo "HAVE_LINUX_DDE_BASE=n" >> $BUILD_DIR_ARM/.defconfig
make O=$BUILD_DIR_ARM ARCH=arm BOARD=goldfishv7 defconfig
UCORE_TEST=xx make O=$BUILD_DIR_ARM sfsimg
make O=$BUILD_DIR_ARM kernel

popd > /dev/null
