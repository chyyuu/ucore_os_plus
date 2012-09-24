#!/bin/bash

pushd `dirname $0` > /dev/null
source autotest.config
cd ucore

# Run tests on i386 silently
./uCore_test -d $BUILD_DIR_I386 -t 20 -r -s ucore > /dev/null 2>&1

# Run tests on arm silently
./uCore_test -t 45 -w 15 -d $BUILD_DIR_ARM -r -s all > /dev/null 2>&1

# Dump results to stdout
echo "==================== i386 ===================="
cat $BUILD_DIR_I386/test-result.latest/summary
echo ""
echo "==================== arm  ===================="
cat $BUILD_DIR_ARM/test-result.latest/summary

popd > /dev/null
