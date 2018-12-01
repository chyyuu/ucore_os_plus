use riscv::register::*;

#[derive(Debug, Clone)]
#[repr(C)]
pub struct TrapFrame {
    pub x: [usize; 32],
    pub sstatus: sstatus::Sstatus,
    pub sepc: usize,
    pub sbadaddr: usize,
    pub scause: scause::Scause,
}