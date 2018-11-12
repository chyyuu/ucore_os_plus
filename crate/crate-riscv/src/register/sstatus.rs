//! sstatus register
// TODO: Virtualization, Memory Privilege and Extension Context Fields

use bit_field::BitField;

/// Supervisor Status Register
#[derive(Clone, Copy, Debug)]
pub struct Sstatus {
    bits: usize,
}

/// Supervisor Previous Privilege Mode
#[derive(Eq, PartialEq)]
pub enum SPP {
    Supervisor = 1,
    User = 0,
}

impl Sstatus {
    /// User Interrupt Enable
    #[inline(always)]
    pub fn uie(&self) -> bool {
        self.bits & (1 << 0) == 1 << 0
    }

    /// Supervisor Interrupt Enable
    #[inline(always)]
    pub fn sie(&self) -> bool {
        self.bits & (1 << 1) == 1 << 1
    }

    /// User Previous Interrupt Enable
    #[inline(always)]
    pub fn upie(&self) -> bool {
        self.bits & (1 << 4) == 1 << 4
    }

    /// Supervisor Previous Interrupt Enable
    #[inline(always)]
    pub fn spie(&self) -> bool {
        self.bits & (1 << 5) == 1 << 5
    }

    /// Supervisor Previous Privilege Mode
    #[inline(always)]
    pub fn spp(&self) -> SPP {
        match self.bits & (1 << 8) == (1 << 8) {
            true => SPP::Supervisor,
            false => SPP::User,
        }
    }

    /// Make eXecutable Readable
    #[inline(always)]
    pub fn mxr(&self) -> bool {
        self.bits.get_bit(19)
    }

    /// Permit Supervisor User Memory access
    #[inline(always)]
    pub fn sum(&self) -> bool {
        self.bits.get_bit(18)
    }

    #[inline(always)]
    pub fn set_spie(&mut self, val: bool) {
        self.bits.set_bit(5, val);
    }

    #[inline(always)]
    pub fn set_sie(&mut self, val: bool) {
        self.bits.set_bit(1, val);
    }

    #[inline(always)]
    pub fn set_spp(&mut self, val: SPP) {
        self.bits.set_bit(8, val == SPP::Supervisor);
    }
}

read_csr_as!(Sstatus, 0x100);
set!(0x100);
clear!(0x100);

/// User Interrupt Enable
set_clear_csr!(set_uie, clear_uie, 1 << 0);
/// Supervisor Interrupt Enable
set_clear_csr!(set_sie, clear_sie, 1 << 1);
/// User Previous Interrupt Enable
set_csr!(set_upie, 1 << 4);
/// Supervisor Previous Interrupt Enable
set_csr!(set_spie, 1 << 5);
/// Make eXecutable Readable
set_clear_csr!(set_mxr, clear_mxr, 1 << 19);
/// Permit Supervisor User Memory access
set_clear_csr!(set_sum, clear_sum, 1 << 18);
/// Supervisor Previous Privilege Mode
#[inline(always)]
pub unsafe fn set_spp(spp: SPP) {
    _set((spp as usize) << 8);
}
