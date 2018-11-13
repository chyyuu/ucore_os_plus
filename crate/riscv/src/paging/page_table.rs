use addr::*;
use core::ops::{Index, IndexMut};
use core::fmt::{Debug, Formatter, Error};

pub struct PageTable {
    entries: [PageTableEntry; ENTRY_COUNT],
}

impl PageTable {
    /// Clears all entries.
    pub fn zero(&mut self) {
        for entry in self.entries.iter_mut() {
            entry.set_unused();
        }
    }

    /// Virtual address of root: (R, R+1, 0)
    pub fn set_recursive(&mut self, recursive_index: usize, frame: Frame) {
        type EF = PageTableFlags;
        self[recursive_index].set(frame.clone(), EF::VALID);
        self[recursive_index + 1].set(frame.clone(), EF::VALID | EF::READABLE | EF::WRITABLE);
    }

    /// Setup identity map: VirtPage at pagenumber -> PhysFrame at pagenumber
    /// pn: pagenumber = addr>>12 in riscv32.
    pub fn map_identity(&mut self, pn: usize, flags: PageTableFlags) {
        self.entries[pn].set(Frame::of_addr(PhysAddr::new((pn as u32) << 22)), flags);
    }
}

impl Index<usize> for PageTable {
    type Output = PageTableEntry;

    fn index(&self, index: usize) -> &Self::Output {
        &self.entries[index]
    }
}

impl IndexMut<usize> for PageTable {
    fn index_mut(&mut self, index: usize) -> &mut Self::Output {
        &mut self.entries[index]
    }
}

impl Debug for PageTable {
    fn fmt(&self, f: &mut Formatter) -> Result<(), Error> {
        f.debug_map()
            .entries(self.entries.iter().enumerate()
                .filter(|p| !p.1.is_unused()))
            .finish()
    }
}

#[derive(Copy, Clone)]
pub struct PageTableEntry(u32);

impl PageTableEntry {
    pub fn is_unused(&self) -> bool {
        self.0 == 0
    }
    pub fn set_unused(&mut self) {
        self.0 = 0;
    }
    pub fn flags(&self) -> PageTableFlags {
        PageTableFlags::from_bits_truncate(self.0)
    }
    pub fn addr(&self) -> PhysAddr {
        PhysAddr::new((self.0 << 2) & 0xfffff000)
    }
    pub fn frame(&self) -> Frame {
        Frame::of_addr(self.addr())
    }
    pub fn set(&mut self, frame: Frame, flags: PageTableFlags) {
        self.0 = (frame.number() << 10) as u32 | flags.bits();
    }
    pub fn flags_mut(&mut self) -> &mut PageTableFlags {
        unsafe { &mut *(self as *mut _ as *mut PageTableFlags) }
    }
}

impl Debug for PageTableEntry {
    fn fmt(&self, f: &mut Formatter) -> Result<(), Error> {
        f.debug_struct("PageTableEntry")
            .field("frame", &self.frame())
            .field("flags", &self.flags())
            .finish()
    }
}

const ENTRY_COUNT: usize = 1 << 10;

bitflags! {
    /// Possible flags for a page table entry.
    pub struct PageTableFlags: u32 {
        const VALID =       1 << 0;
        const READABLE =    1 << 1;
        const WRITABLE =    1 << 2;
        const EXECUTABLE =  1 << 3;
        const USER =        1 << 4;
        const GLOBAL =      1 << 5;
        const ACCESSED =    1 << 6;
        const DIRTY =       1 << 7;
        const RESERVED1 =   1 << 8;
        const RESERVED2 =   1 << 9;
    }
}
