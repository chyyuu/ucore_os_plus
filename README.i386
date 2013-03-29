README for ucore i386 in ubuntu 12.04 x86-64

-----------------------------
prerequirement:
------------------
intalll gcc x86-64 and qemu-system-x86_64

------------------
build
------------------
cd ucore
make menuconfig ARCH=x86_84
make
make sfsimg
make swapimg

------------------
run
------------------
./uCore_run -d obj

OR
qemu-system-i386 -m 128 -hda obj//kernel.img -drive file=obj//swap.img,media=disk,cache=writeback -drive file=obj//sfs.img,media=disk,cache=writeback -s -serial file:obj//serial.log -monitor stdio

