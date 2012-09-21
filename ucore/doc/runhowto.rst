===================
Build and Run uCore
===================

:Author: Mao Junjie <eternal.n08@gmail.com>
:Version: $Revision: 2 $

This document discusses how to build ucore for different architectures and the usage of scripts for running and testing the system.

.. contents::

Building uCore
==============

To build ucore, type the following commands.

    make ARCH=xxx BOARD=yyy defconfig

    make sfsimg

    make kernel

    make swapimg (not for arm)

    bash uCore_run

These commands will generate all the files needed under obj/.

Variables
=========

ARCH=<arch>
  What architecture **ucore** is built for. Possible choices of <arch> include **i386** and **arm**.

BOARD=<board>
  Some architectures have multiple kinds of boards that have different chipset, devices or something else. To build uCore for a specific board, this variable is needed. At present it is required that BOARD=goldfishv7 is given when ARCH=arm.

O=<dir>
  When defined, the object files will be generated under <dir> instead of obj/. This is useful when you want to keep multiple copy of uCore builds with different configurations.

Targets
=======
defconfig
  Configure the build options with a default config script.

menuconfig
  Start a menu screen implemented by curses for adjusting build options.

kernel
  Build the kernel image.

sfsimg
  Build the simple filesystem image.

swapimg
  Build the swap image. This is not necessary for architectures without swap support such as arm.

clean
  Clean everything.

Running uCore
=============
The uCore_run script is used to run the system. It reads a build directory and invokes the simulator according to the architecture the build is for. A typical usage of this script is

bash uCore_run -d <dir>

where <dir> is the build directory.

Configuration required for running uCore for arm
------------------------------------------------

Before running uCore for arm, it is needed to create an AVD in Android SDK. The CPU/ABI should be "ARM (arm-eabi v7a)" and the RAM size if 128M. uCore_run will fetch the first AVD from "android avd list" by default. If your AVD for uCore is not the first one listed, use the environment variable AVD to override this, i.e.

AVD=xxx bash uCore_run ...

where xxx is the name of your AVD.

Testing uCore
=============
The uCore_test script is used to automatically carry out a series of tests. Here's some typical invocation of the script.

bash uCore_test -d <dir>
  Running all tests from ucore/src/user-ucore/testspecs, using the uCore build in <dir>, and print the results on the terminal.

bash uCore_test -d <dir> -s <testsuite>
  Running all testsuites listed. The "-s" option can be used multiple times. Use "bash uCore_test -h" for supported testsuites.

bash uCore_test -d <dir> -f <testspec> -s <testsuite>
  Running the test given by <testspec> in <testsuite> using the uCore build in <dir>.
