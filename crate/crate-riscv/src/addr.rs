use bit_field::BitField;

#[derive(Debug, Copy, Clone, PartialEq, Eq, PartialOrd, Ord)]
pub struct VirtAddr(usize);

impl VirtAddr {
    pub fn new(addr: usize) -> VirtAddr {
        VirtAddr(addr)
    }
    pub fn as_usize(&self) -> usize {
        self.0
    }
    pub fn p2_index(&self) -> usize {
        self.0.get_bits(22..32)
    }
    pub fn p1_index(&self) -> usize {
        self.0.get_bits(12..22)
    }
    pub fn page_number(&self) -> usize {
        self.0.get_bits(12..32)
    }
    pub fn page_offset(&self) -> usize {
        self.0.get_bits(0..12)
    }
    pub(crate) unsafe fn as_mut<'a, 'b, T>(&'a self) -> &'b mut T {
        &mut *(self.0 as *mut T)
    }
}

#[derive(Debug, Copy, Clone, PartialEq, Eq, PartialOrd, Ord)]
pub struct PhysAddr(u32);

impl PhysAddr {
    pub fn new(addr: u32) -> PhysAddr {
        PhysAddr(addr)
    }
    pub fn as_u32(&self) -> u32 {
        self.0
    }
    pub fn p2_index(&self) -> usize {
        self.0.get_bits(22..32) as usize
    }
    pub fn p1_index(&self) -> usize {
        self.0.get_bits(12..22) as usize
    }
    pub fn page_number(&self) -> usize {
        self.0.get_bits(12..32) as usize
    }
    pub fn page_offset(&self) -> usize {
        self.0.get_bits(0..12) as usize
    }
}

#[derive(Debug, Copy, Clone, PartialEq, Eq, PartialOrd, Ord)]
pub struct Page(VirtAddr);

impl Page {
    pub fn of_addr(addr: VirtAddr) -> Self {
        Page(VirtAddr(addr.0 & 0xfffff000))
    }
    pub fn start_address(&self) -> VirtAddr {
        self.0.clone()
    }
    pub fn p2_index(&self) -> usize {
        self.0.p2_index()
    }
    pub fn p1_index(&self) -> usize {
        self.0.p1_index()
    }
    pub fn number(&self) -> usize {
        self.0.page_number()
    }
    pub fn from_page_table_indices(p2_index: usize, p1_index: usize) -> Self {
        use bit_field::BitField;
        let mut addr = 0;
        addr.set_bits(22..32, p2_index);
        addr.set_bits(12..22, p1_index);
        Page::of_addr(VirtAddr::new(addr))
    }
}

#[derive(Debug, Copy, Clone, PartialEq, Eq, PartialOrd, Ord)]
pub struct Frame(PhysAddr);

impl Frame {
    pub fn of_addr(addr: PhysAddr) -> Self {
        Frame(PhysAddr(addr.0 & 0xfffff000))
    }
    pub fn start_address(&self) -> PhysAddr {
        self.0.clone()
    }
    pub fn p2_index(&self) -> usize {
        self.0.p2_index()
    }
    pub fn p1_index(&self) -> usize {
        self.0.p1_index()
    }
    pub fn number(&self) -> usize {
        self.0.page_number()
    }
}