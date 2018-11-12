//! sie register

/// sie register
#[derive(Clone, Copy, Debug)]
pub struct Sie {
    bits: usize,
}

impl Sie {
    /// Returns the contents of the register as raw bits
    #[inline(always)]
    pub fn bits(&self) -> usize {
        self.bits
    }

    /// User Software Interrupt Enable
    #[inline(always)]
    pub fn usoft(&self) -> bool {
        self.bits & (1 << 0) == 1 << 0
    }

    /// Supervisor Software Interrupt Enable
    #[inline(always)]
    pub fn ssoft(&self) -> bool {
        self.bits & (1 << 1) == 1 << 1
    }

    /// User Timer Interrupt Enable
    #[inline(always)]
    pub fn utimer(&self) -> bool {
        self.bits & (1 << 4) == 1 << 4
    }

    /// Supervisor Timer Interrupt Enable
    #[inline(always)]
    pub fn stimer(&self) -> bool {
        self.bits & (1 << 5) == 1 << 5
    }

    /// User External Interrupt Enable
    #[inline(always)]
    pub fn uext(&self) -> bool {
        self.bits & (1 << 8) == 1 << 8
    }

    /// Supervisor External Interrupt Enable
    #[inline(always)]
    pub fn sext(&self) -> bool {
        self.bits & (1 << 9) == 1 << 9
    }
}

read_csr_as!(Sie, 0x104);
set!(0x104);
clear!(0x104);

/// User Software Interrupt Enable
set_clear_csr!(set_usoft, clear_usoft, 1 << 0);
/// Supervisor Software Interrupt Enable
set_clear_csr!(set_ssoft, clear_ssoft, 1 << 1);
/// User Timer Interrupt Enable
set_clear_csr!(set_utimer, clear_utimer, 1 << 4);
/// Supervisor Timer Interrupt Enable
set_clear_csr!(set_stimer, clear_stimer, 1 << 5);
/// User External Interrupt Enable
set_clear_csr!(set_uext, clear_uext, 1 << 8);
/// Supervisor External Interrupt Enable
set_clear_csr!(set_sext, clear_sext, 1 << 9);
