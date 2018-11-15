use spin::Once;
use sync::{SpinNoIrqLock, Mutex, MutexGuard, SpinNoIrq};
pub use self::context::Context;
pub use ucore_process::processor::{*, Context as _whatever};
pub use ucore_process::scheduler::*;
pub use ucore_process::thread::*;

mod context;

type Processor = Processor_<Context, StrideScheduler>;

// Hard link user program
// Need to build user process first!
global_asm!(r#"
    .section .rodata
    .global _hello_bin_start
    .global _hello_bin_end
_hello_bin_start:
    .incbin "../user/target/riscv32-ucore/debug/hello"
_hello_bin_end:
"#);

extern {
    fn _hello_bin_start();
    fn _hello_bin_end();
}

pub fn init() {
    PROCESSOR.call_once(||
        SpinNoIrqLock::new({
            let mut processor = Processor::new(
                unsafe { Context::new_init() },
                // NOTE: max_time_slice <= 5 to ensure 'priority' test pass
                StrideScheduler::new(5),
            );
            extern fn idle(arg: usize) -> ! {
                loop {}
            }
            processor.add(Context::new_kernel(idle, 0));
            let hello_bin_data = unsafe { ::core::slice::from_raw_parts(_hello_bin_start as usize as *const u8, _hello_bin_end as usize - _hello_bin_start as usize) };
            processor.add(Context::new_user(hello_bin_data));

            processor
        })
    );
    info!("process init end");
}

pub static PROCESSOR: Once<SpinNoIrqLock<Processor>> = Once::new();

pub fn processor() -> MutexGuard<'static, Processor, SpinNoIrq> {
    PROCESSOR.try().unwrap().lock()
}

#[allow(non_camel_case_types)]
pub type thread = ThreadMod<ThreadSupportImpl>;

pub mod thread_ {
    pub type Thread = super::Thread<super::ThreadSupportImpl>;
}

pub struct ThreadSupportImpl;

impl ThreadSupport for ThreadSupportImpl {
    type Context = Context;
    type Scheduler = StrideScheduler;
    type ProcessorGuard = MutexGuard<'static, Processor, SpinNoIrq>;

    fn processor() -> Self::ProcessorGuard {
        processor()
    }
}