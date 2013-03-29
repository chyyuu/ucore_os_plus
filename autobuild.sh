#!/bin/bash

pushd `dirname $0` > /dev/null
source autotest.config
cd ucore

# Remove previous builds first
rm -rf $BUILD_DIR_I386
rm -rf $BUILD_DIR_ARM
rm -rf $BUILD_DIR_AMD64

# Build uCore for i386
make O=$BUILD_DIR_I386 ARCH=i386 defconfig > /dev/null && \
make O=$BUILD_DIR_I386 kernel > /dev/null && \
UCORE_TEST=xx make O=$BUILD_DIR_I386 sfsimg > /dev/null
if [ $? -ne 0 ]; then
    echo "build uCore for i386 failed!"
    exit 1
fi
echo "build uCore for i386: OK"

# Build uCore for arm on goldfishv7
# Disable DDE at present as it violates the checks after all user-mode processes quit
mkdir -p $BUILD_DIR_ARM  > /dev/null && \
grep -v DDE src/kern-ucore/arch/arm/configs/goldfishv7_defconfig > $BUILD_DIR_ARM/.defconfig && \
echo "HAVE_LINUX_DDE_BASE=n" >> $BUILD_DIR_ARM/.defconfig && \
make O=$BUILD_DIR_ARM ARCH=arm BOARD=goldfishv7 defconfig > /dev/null && \
UCORE_TEST=xx make O=$BUILD_DIR_ARM sfsimg > /dev/null && \
make O=$BUILD_DIR_ARM kernel > /dev/null
if [ $? -ne 0 ]; then
    echo "build uCore for arm failed!"
    exit 1
fi
echo "build uCore for arm: OK"

# Build uCore for amd64
make O=$BUILD_DIR_AMD64 ARCH=amd64 defconfig > /dev/null && \
make O=$BUILD_DIR_AMD64 kernel > /dev/null && \
UCORE_TEST=xx make O=$BUILD_DIR_AMD64 sfsimg > /dev/null
if [ $? -ne 0 ]; then
    echo "build uCore for amd64 failed!"
    exit 1
fi
echo "build uCore for amd64: OK"

popd > /dev/null
