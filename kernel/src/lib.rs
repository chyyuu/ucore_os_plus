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

// println! process:
// $crate::logging::println!-->$crate::logging::print!-->crate::arch::io::putfmt(args)
// -->crate::arch::io::SerialPort.write_fmt(FROM trait Write) --> trait Write.write_str-->putchar
// --> sbi::console_putchar(c as usize)

    println!("Hello World{}", "!");
    loop {}
}
