README for ucore amd64 in ubuntu 12.04 x86-64

-----------------------------
prerequirement:
------------------
intalll gcc x86-64 and qemu-system-x86_64

------------------
build
------------------
cd ucore
1 set kernel configure 
  make ARCH=amd64 defconfig      // use amd64's default configure
 OR
  make ARCH=amd64 menuconfig     //do nothing or select custom config, just save&exit
2 build kernel
 make                            // build obj/kernel.img            
3 build file system 
make sfsimg                      // build obj/sfs.img

NOTICE: no need to build swapimg
------------------
run
------------------
./uCore_run -d obj

OR
qemu-system-x86_64 -m 512 -hda obj/kernel.img -drive file=obj/sfs.img,media=disk,cache=writeback,index=2 -s -serial file:obj/serial.log -monitor stdio

------------------
debug
------------------
* qemu+gdb

  qemu and gdb do not properly handle mode-switching between 32- and
  64-bit code: once gdb connects in 32-bit mode, it will not switch to
  64-bit mode.  Thus, debugging 64-bit code requires attaching gdb after
  qemu is already executing 64-bit code.  Do not use -S, since that will
  attach in 16-/32-bit mode.

  //In a terminal..
  $ uCore_run -d obj 

  //In a different terminal
  $ cp gdbinit.amd64 .gdbinit
  $ gdb
  ...
  The target architecture is assumed to be i386:x86-64
  0xffffffffc011b5fc in ?? ()
  (gdb) continue

