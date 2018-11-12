//! satp register

use addr::*;
use bit_field::BitField;

/// satp register
#[derive(Clone, Copy, Debug)]
pub struct Satp {
    bits: usize,
}

impl Satp {
    /// Returns the contents of the register as raw bits
    #[inline(always)]
    pub fn bits(&self) -> usize {
        self.bits
    }

    /// Current address-translation scheme
    #[cfg(target_pointer_width = "32")]
    #[inline(always)]
    pub fn mode(&self) -> Mode {
        match self.bits.get_bit(31) {
            false => Mode::Bare,
            true => Mode::Sv32,
        }
    }

    /// Current address-translation scheme
    #[cfg(target_pointer_width = "64")]
    #[inline(always)]
    pub fn mode(&self) -> Mode {
        match self.bits.get_bits(60..64) {
            0 => Mode::Bare,
            8 => Mode::Sv39,
            9 => Mode::Sv48,
            10 => Mode::Sv57,
            11 => Mode::Sv64,
            _ => panic!("invalid satp mode"),
        }
    }

    /// Address space identifier
    #[cfg(target_pointer_width = "32")]
    #[inline(always)]
    pub fn asid(&self) -> usize {
        self.bits.get_bits(22..31)
    }

    /// Address space identifier
    #[cfg(target_pointer_width = "64")]
    #[inline(always)]
    pub fn asid(&self) -> usize {
        self.bits.get_bits(44..60)
    }

    /// Physical page number
    #[cfg(target_pointer_width = "32")]
    #[inline(always)]
    pub fn ppn(&self) -> usize {
        self.bits.get_bits(0..22)
    }

    /// Physical page number
    #[cfg(target_pointer_width = "64")]
    #[inline(always)]
    pub fn ppn(&self) -> usize {
        self.bits.get_bits(0..44)
    }

    /// Physical frame
    #[cfg(target_pointer_width = "32")]
    #[inline(always)]
    pub fn frame(&self) -> Frame {
        Frame::of_addr(PhysAddr::new((self.ppn() as u32) << 12))
    }
}

#[cfg(target_pointer_width = "32")]
pub enum Mode {
    Bare = 0, Sv32 = 1,
}

#[cfg(target_pointer_width = "64")]
pub enum Mode {
    Bare = 0, Sv39 = 8, Sv48 = 9, Sv57 = 10, Sv64 = 11,
}

read_csr_as!(Satp, 0x180);
write_csr!(0x180);

#[inline(always)]
#[cfg(target_pointer_width = "32")]
pub unsafe fn set(mode: Mode, asid: usize, frame: Frame) {
    let mut bits = 0usize;
    bits.set_bits(31..32, mode as usize);
    bits.set_bits(22..31, asid);
    bits.set_bits(0..22, frame.number());
    _write(bits);
}