#![feature(lang_items)]
#![feature(panic_info_message)]
#![feature(global_asm)]
#![no_std]

#[macro_use]
extern crate log;

#[macro_use]
pub mod logging;

mod lang;

#[cfg(target_arch = "riscv32")]
#[path = "arch/riscv32/mod.rs"]
pub mod arch;

/// The entry point of Rust kernel
#[no_mangle]
pub extern "C" fn rust_main() -> ! {
    println!("Hello World{}", "!");
    loop {}
}
