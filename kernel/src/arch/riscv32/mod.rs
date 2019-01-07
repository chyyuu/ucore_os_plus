extern crate riscv;
extern crate bbl;

pub mod io;
pub mod compiler_rt;

#[cfg(feature = "no_bbl")]
//without boot.asm, qemu can not run correctly???
global_asm!(include_str!("boot/boot.asm"));
global_asm!(include_str!("boot/entry.asm"));