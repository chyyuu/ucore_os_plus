.set noreorder
.text
.globl _start
.extern _gp
_start:
    nop
    la $gp, _gp
    addiu $sp, $sp, -16
    jal umain
    nop

