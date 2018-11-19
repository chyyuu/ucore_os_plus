use core::fmt;

#[macro_export]
macro_rules! print {
    ($($arg:tt)*) => ({
        $crate::logging::print(format_args!($($arg)*));
    });
}

#[macro_export]
macro_rules! println {
    ($fmt:expr) => (print!(concat!($fmt, "\n")));
    ($fmt:expr, $($arg:tt)*) => (print!(concat!($fmt, "\n"), $($arg)*));
}

pub fn print(args: fmt::Arguments) {
    use crate::arch::io;
    io::putfmt(args);
}