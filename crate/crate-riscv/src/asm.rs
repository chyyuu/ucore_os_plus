//! Assembly instructions

macro_rules! instruction {
    ($fnname:ident, $asm:expr) => (
        #[inline]
        pub unsafe fn $fnname() {
            match () {
                #[cfg(any(target_arch = "riscv32", target_arch = "riscv64"))]
                () => asm!($asm :::: "volatile"),
                #[cfg(not(any(target_arch = "riscv32", target_arch = "riscv64")))]
                () => {}
            }
        }
    )
}


/// Priviledged ISA Instructions
instruction!(ecall, "ecall");
instruction!(ebreak, "ebreak");
instruction!(uret, "uret");
instruction!(sret, "sret");
instruction!(mret, "mret");
instruction!(wfi, "wfi");

use addr::VirtAddr;

#[inline(always)]
pub fn sfence_vma_all() {
    unsafe{ asm!("sfence.vma" :::: "volatile"); }
}

#[inline(always)]
pub fn sfence_vma(asid: usize, addr: VirtAddr) {
    unsafe{ asm!("sfence.vma $0, $1" :: "r"(asid), "r"(addr.as_usize()) :: "volatile"); }
}