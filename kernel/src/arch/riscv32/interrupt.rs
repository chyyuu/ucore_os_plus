use riscv::register::*;
pub use self::context::*;
use log::*;

#[path = "context.rs"]
mod context;

pub fn init() {
    extern {
        fn __alltraps();
    }
    unsafe {
        // Set sscratch register to 0, indicating to exception vector that we are
        // presently executing in the kernel
        sscratch::write(0);
        // Set the exception vector address
        stvec::write(__alltraps as usize, stvec::TrapMode::Direct);
    }
    info!("interrupt: init end");
}

#[inline(always)]
pub unsafe fn enable() {
    sstatus::set_sie();
}

#[inline(always)]
pub unsafe fn disable_and_store() -> usize {
    let e = sstatus::read().sie() as usize;
    sstatus::clear_sie();
    e
}

#[inline(always)]
pub unsafe fn restore(flags: usize) {
    if flags != 0 {
        sstatus::set_sie();
    }
}

// trap.asm::__alltraps --> rust_trap --> trap.asm::__trapret
#[no_mangle]
pub extern fn rust_trap(tf: &mut TrapFrame) {
    use riscv::register::scause::{Trap, Interrupt as I, Exception as E};
    trace!("Interrupt: {:?}", tf.scause.cause());
    match tf.scause.cause() {
        Trap::Interrupt(I::SupervisorTimer) => timer(),
        Trap::Exception(E::IllegalInstruction) => trace!("Error: Illegal Instr"),
        _ => trace!("Error Interrupt"),
    }
    //::trap::before_return();
    trace!("Interrupt end");
}

fn timer() {
    super::timer::set_next();
    println!("tick");
}