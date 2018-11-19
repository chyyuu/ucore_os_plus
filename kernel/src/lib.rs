#![feature(ptr_internals)]
#![feature(lang_items)]
#![feature(const_fn)]
#![feature(alloc)]
#![feature(allocator_api)]
#![feature(unboxed_closures)]
#![feature(naked_functions)]
#![feature(asm)]
#![feature(optin_builtin_traits)]
#![feature(panic_info_message)]
#![feature(global_asm)]
#![feature(compiler_builtins_lib)]
#![feature(extern_crate_item_prelude)]
#![no_std]

extern crate alloc;

use linked_list_allocator::LockedHeap;

#[macro_use]    // print!
mod logging;
mod memory;
mod lang;
mod util;
mod consts;
mod process;
mod syscall;

use process::{thread, thread_};
mod sync;
mod trap;

#[cfg(target_arch = "riscv32")]
#[path = "arch/riscv32/mod.rs"]
pub mod arch;

/// The entry point of Rust kernel
#[no_mangle]
pub extern "C" fn rust_main() -> ! {
    println!("Hello World{}", "!");
    logging::init();
    arch::init(); //include memory::init()
    process::init();
    unsafe { arch::interrupt::enable(); }

    //thread::test::local_key();

    loop {}
}

/// Global heap allocator
/// Available after `memory::init()`.
/// It should be defined in memory mod, but in Rust `global_allocator` must be in root mod.
#[global_allocator]
static HEAP_ALLOCATOR: LockedHeap = LockedHeap::empty();