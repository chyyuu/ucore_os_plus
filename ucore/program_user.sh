bin2flash --input="obj/sfs-orig.img" --output="obj/sfs.flash" --location=0x100000 --verbose 
nios2-flash-programmer obj/sfs.flash --cable=1 --base=0xc0000000  --program --verbose -g --cable=1
nios2-flash-programmer obj/sfs.flash --cable=1 --base=0xc0000000  --program --verbose -g --cable=2
