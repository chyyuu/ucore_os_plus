uCore_plus
==========

##Makefile
**Cross Compile**

set the environment variables:

export ARCH = ?

export CROSS\_COMPILE = ?

(see Makefile in ./ucore)

**Kconfig**

The top level Kconfig is ./ucore/arch/xxx/Kconfig. You can include other Kconfig

  (see ./ucore/arch/arm/Kconfig)

  All config option is prefixed with UCONFIG_

**Makefile**

Supported variables:  obj-y, dirs-y

(See ./ucore/kern-ucore/Makefile.build and Makefile.subdir)

**Add a new ARCH**

In arch/xxx, add Makefile.image and include.mk

***include.mk: define ARCH_INCLUDES and ARCH_CFLAGD, etc.

***Makefile.image: how to generate the executable for your platform.

***Kconfig: your top level Kconfig

***Makefile: recursively add Makefile in your arch directory.

More Document
========
See ucore/doc

