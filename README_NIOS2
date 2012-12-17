To build ucore for nios2, quartus must be installed.

In nios2 command shell, execute the following commands to build ucore:
	make ARCH=nios2 menuconfig
	make kernel
	make sfsimg
	
Note that sysimg can't be more than 3MB, so minigui must be removed from sysimg. You must comment the following line in Makefile before "make sysimg":
	-cp -r $(TOPDIR)/src/user-ucore/_initial/* $(TMPSFS)
	
execute program.sh and program_user.sh to program the kernel and sysimg into flash. 

