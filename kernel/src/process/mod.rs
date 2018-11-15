use spin::Once;
use sync::{SpinNoIrqLock, Mutex, MutexGuard, SpinNoIrq};
pub use self::context::Context;
pub use ucore_process::processor::{*, Context as _whatever};
pub use ucore_process::scheduler::*;
pub use ucore_process::thread::*;
use core::time::Duration;

mod context;

type Processor = Processor_<Context, StrideScheduler>;

pub fn init() {
    PROCESSOR.call_once(||
        SpinNoIrqLock::new({
            let mut processor = Processor::new(
                unsafe { Context::new_init() },
                // NOTE: max_time_slice <= 5 to ensure 'priority' test pass
                StrideScheduler::new(5),
            );
            processor.add(Context::new_kernel(thread1, 0));
            processor.add(Context::new_kernel(thread2, 0));
            processor
        })
    );
    info!("process init end");
}
extern fn thread1(arg: usize) -> ! {
    loop {
        println!("I'm thread 1.");
        spin_sleep();
    }
}

extern fn thread2(arg: usize) -> ! {
    loop {
        println!("I'm thread 2.");
        spin_sleep();
    }
}

fn spin_sleep() {
    for i in 0..500000 {}
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