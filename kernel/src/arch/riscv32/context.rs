use riscv::register::*;

#[derive(Debug, Clone)]
#[repr(C)]
pub struct TrapFrame {
    pub x: [usize; 32],
    pub sstatus: sstatus::Sstatus,
    pub sepc: usize,
    pub sbadaddr: usize,
    pub scause: scause::Scause,
}

#[derive(Debug)]
pub struct Context(usize);

impl Context {
    /// Switch to another kernel thread.
    ///
    /// Defined in `trap.asm`.
    ///
    /// Push all callee-saved registers at the current kernel stack.
    /// Store current sp, switch to target.
    /// Pop all callee-saved registers, then return to the target.
    #[naked]
    #[inline(never)]
    pub unsafe extern fn switch(&mut self, _target: &mut Self) {
        asm!(
        "
        // save from's registers
        addi sp, sp, -4*14
        sw sp, 0(a0)
        sw ra, 0*4(sp)
        sw s0, 2*4(sp)
        sw s1, 3*4(sp)
        sw s2, 4*4(sp)
        sw s3, 5*4(sp)
        sw s4, 6*4(sp)
        sw s5, 7*4(sp)
        sw s6, 8*4(sp)
        sw s7, 9*4(sp)
        sw s8, 10*4(sp)
        sw s9, 11*4(sp)
        sw s10, 12*4(sp)
        sw s11, 13*4(sp)
        csrrs s11, 0x180, x0 // satp
        sw s11, 1*4(sp)

        // restore to's registers
        lw sp, 0(a1)
        lw s11, 1*4(sp)
        csrrw x0, 0x180, s11 // satp
        lw ra, 0*4(sp)
        lw s0, 2*4(sp)
        lw s1, 3*4(sp)
        lw s2, 4*4(sp)
        lw s3, 5*4(sp)
        lw s4, 6*4(sp)
        lw s5, 7*4(sp)
        lw s6, 8*4(sp)
        lw s7, 9*4(sp)
        lw s8, 10*4(sp)
        lw s9, 11*4(sp)
        lw s10, 12*4(sp)
        lw s11, 13*4(sp)
        addi sp, sp, 4*14

        sw zero, 0(a1)
        ret"
        : : : : "volatile" )
    }

    pub unsafe fn null() -> Self {
        Context(0)
    }
}