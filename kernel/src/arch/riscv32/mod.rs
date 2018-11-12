extern crate riscv;
extern crate bbl;

pub mod io;
pub mod compiler_rt;

#[cfg(feature = "no_bbl")]
global_asm!(include_str!("boot/boot.asm"));
global_asm!(include_str!("boot/entry.asm"));
//global_asm!(include_str!("boot/trap.asm"));