=====================================
uCore Environment Establishment HOWTO
=====================================

:Author: Mao Junjie <eternal.n08@gmail.com>
:Version: $Revision: 1 $

This document gives a step-by-step howto on establishing the environment for running and testing uCore.

.. contents::

uCore for i386
==============

Here is the list of required tools for building, running and testing uCore for i386.

Building
--------
binutils and gcc for x86
  Either x86_32 or x86_64 is ok. You can install the toolchain by the package manager in your distribution, such as yum in Fedora and apt-get in Ubuntu. To verify if the toolchain has been installed successfuly, run

    | gcc -v

  and you should see the version info of your gcc.

GNU make
  This is mostly installed with your system by default. Run

    | make -v

  to see if it is installed properly. If not, please consult your package manager in your distribution.

Running
-------
Qemu which simulates i386
  This is also available via the package manager. The package may be called *qemu* or *qemu-system-i386*. To verify your installation, run

    | qemu -cpu ?

  or

    | qemu-system-i386 -cpu ?

  and you should see some lines like

    | x86           [n270]
    | x86         [athlon]
    | ....

  but not

    | x86      SandyBridge
    | x86         Westmere
    | ....

Testing
-------
gdb for x86
  This is available via the package manager. The package may be called *gdb*. To verify your installation, run

    | gdb -v

  and you should see a line like

    | This GDB was configured as "x86_64-......

uCore for arm
=============

Here is the list of required tools for building, running and testing uCore for i386.

Building
--------

.. _install-arm-toolchain-for-kernel:

binutils and gcc for the kernel
  You can download the tools at [#]_. After you download the tarball, run

    | mkdir -p ~/tools
    | cd ~/tools
    | tar xvf ~/Downloads/arm-eabi-4.4.3-linux-x86.tar.gz
    | export PATH=$PATH:~/tools/arm-eabi-4.4.3/bin

  and verify your installation by

    | arm-eabi-gcc -v

  You should see the following lines at the end of the output

    | Using built-in specs
    | Target: arm-eabi
    | ...
    | gcc version 4.4.3 (GCC)

  Note that you have to export the PATH everytime you start a terminal. To keep the configuration, append the line to the end of ~/.bashrc (for Ubuntu) or ~/.bash_profile (for Fedora).

binutils and gcc for applications
  Download the tools at [#]_ and run the following commands

    | cd ~/tools
    | tar xvf ~/Downloads/arm-linux-androideabi-4.6-linux-x86.tar.gz
    | export PATH=$PATH:~/tools/arm-linux-androideabi-4.6/bin

  and verify your installation by

    | arm-linux-androideabi-gcc -v

  You should see

    | gcc version 4.6.x-google 20120106 (prerelease) (GCC)

  at the end of the output.


Running
-------
Android SDK
  You can download Android SDK from [#]_. After the tarball for your platform is downloaded, run

    | cd ~/tools
    | tar xvf ~/Downloads/android-sdk_r20.0.3-linux.tgz
    | export PATH=$PATH:~/tools/android-sdk/tools
    | android

  Then follow the following steps in the window.

  1. Select Tools->Options... in the menu and fill the "HTTP proxy server" field with "www.google.com".

  2. Select Packages->Reload in the menu.

  3. In the table tick "ARM EABI v7a System Image" and press "Install packages..."

  4. Click Install and wait the installation to be done

  5. Select Tools->Manage AVDs... in the menu.

  6. In the popup window, press "New..."

  7. Fill the field Name with "ucore" and choose the target "Android 4.1 - API Level 16".

  8. In the "Hardware" table, modify the value of "Device ram size" to 128.

  9. Click "Create AVD".

  10. Press ESC on the keyboard and close the window.

  To verify your installation, run

    | emulator-arm -version

  and you should see the following line at the beginning of the output:

    | Android emulator version 20.0.3.0

  Also the following command:

    | android list avd

  should give the following output:

    | Available Android Virtual Devices:
    |     Name: ucore
    |     Path: /home/xxxx/.android/avd/ucore.avd
    |   Target: Android 4.1 (API level 16)
    |      ABI: armeabi-v7a
    |     Skin: WVGA800

Testing
-------
gdb for arm
  This is installed along with the toolchains. See install-arm-toolchain-for-kernel_ for details.

  To check whether it works, run:

    | arm-eabi-gdb -v

  and you should get some version info on the terminal.

.. [#] https://github.com/downloads/chyh1990/ucore-arm-userapp/arm-eabi-4.6-linux-x86.tar.gz
.. [#] https://github.com/downloads/chyyuu/ucore_plus/arm-eabi-4.4.3-linux-x86.tar.gz
.. [#] http://developer.android.com/sdk/index.html
