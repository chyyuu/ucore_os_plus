pub use super::frame_alloc::*;
pub use super::page_table::*;
pub use super::super::addr::*;

pub trait Mapper {
    /// Creates a new mapping in the page table.
    ///
    /// This function might need additional physical frames to create new page tables. These
    /// frames are allocated from the `allocator` argument. At most three frames are required.
    fn map_to<A>(&mut self, page: Page, frame: Frame, flags: PageTableFlags, allocator: &mut A) -> Result<MapperFlush, MapToError>
        where A: FrameAllocator;

    /// Removes a mapping from the page table and returns the frame that used to be mapped.
    ///
    /// Note that no page tables or pages are deallocated.
    fn unmap(&mut self, page: Page) -> Result<(Frame, MapperFlush), UnmapError>;

    /// Return the frame that the specified page is mapped to.
    fn translate_page(&self, page: Page) -> Option<Frame>;

    /// Maps the given frame to the virtual page with the same address.
    fn identity_map<A>(&mut self, frame: Frame, flags: PageTableFlags, allocator: &mut A) -> Result<MapperFlush, MapToError>
        where A: FrameAllocator,
    {
        let page = Page::of_addr(VirtAddr::new(frame.start_address().as_u32() as usize));
        self.map_to(page, frame, flags, allocator)
    }
}

#[must_use = "Page Table changes must be flushed or ignored."]
pub struct MapperFlush(Page);

impl MapperFlush {
    /// Create a new flush promise
    fn new(page: Page) -> Self {
        MapperFlush(page)
    }

    /// Flush the page from the TLB to ensure that the newest mapping is used.
    pub fn flush(self) {
        use super::super::asm::sfence_vma;
        sfence_vma(0, self.0.start_address());
    }

    /// Don't flush the TLB and silence the “must be used” warning.
    pub fn ignore(self) {}
}

/// This error is returned from `map_to` and similar methods.
#[derive(Debug)]
pub enum MapToError {
    /// An additional frame was needed for the mapping process, but the frame allocator
    /// returned `None`.
    FrameAllocationFailed,
    /// An upper level page table entry has the `HUGE_PAGE` flag set, which means that the
    /// given page is part of an already mapped huge page.
    ParentEntryHugePage,
    /// The given page is already mapped to a physical frame.
    PageAlreadyMapped,
}

/// An error indicating that an `unmap` call failed.
#[derive(Debug)]
pub enum UnmapError {
    /// An upper level page table entry has the `HUGE_PAGE` flag set, which means that the
    /// given page is part of a huge page and can't be freed individually.
    ParentEntryHugePage,
    /// The given page is not mapped to a physical frame.
    PageNotMapped,
    /// The page table entry for the given page points to an invalid physical address.
    InvalidFrameAddress(PhysAddr),
}

/// A recursive page table is a last level page table with an entry mapped to the table itself.
///
/// This struct implements the `Mapper` trait.
pub struct RecursivePageTable<'a> {
    p2: &'a mut PageTable,
    recursive_index: usize,
}

/// An error indicating that the given page table is not recursively mapped.
///
/// Returned from `RecursivePageTable::new`.
#[derive(Debug)]
pub struct NotRecursivelyMapped;

impl<'a> RecursivePageTable<'a> {
    /// Creates a new RecursivePageTable from the passed level 2 PageTable.
    ///
    /// The page table must be recursively mapped, that means:
    ///
    /// - The page table must have one recursive entry, i.e. an entry that points to the table
    ///   itself.
    /// - The page table must be active, i.e. the satp register must contain its physical address.
    ///
    /// Otherwise `Err(NotRecursivelyMapped)` is returned.
    pub fn new(table: &'a mut PageTable) -> Result<Self, NotRecursivelyMapped> {
        let page = Page::of_addr(VirtAddr::new(table as *const _ as usize));
        let recursive_index = page.p2_index();

        use super::super::register::satp;
        type F = PageTableFlags;
        if page.p1_index() != recursive_index + 1
            || satp::read().frame() != table[recursive_index].frame()
            || satp::read().frame() != table[recursive_index + 1].frame()
            || !table[recursive_index].flags().contains(F::VALID)
            ||  table[recursive_index].flags().contains(F::READABLE | F::WRITABLE)
            || !table[recursive_index + 1].flags().contains(F::VALID | F::READABLE | F::WRITABLE)
        {
            return Err(NotRecursivelyMapped);
        }

        Ok(RecursivePageTable {
            p2: table,
            recursive_index,
        })
    }

    /// Creates a new RecursivePageTable without performing any checks.
    ///
    /// The `recursive_index` parameter must be the index of the recursively mapped entry.
    pub unsafe fn new_unchecked(table: &'a mut PageTable, recursive_index: usize) -> Self {
        RecursivePageTable {
            p2: table,
            recursive_index,
        }
    }

    fn create_p1_if_not_exist<A>(&mut self, p2_index: usize, allocator: &mut A) -> Result<(), MapToError>
        where A: FrameAllocator,
    {
        type F = PageTableFlags;
        if self.p2[p2_index].is_unused() {
            if let Some(frame) = allocator.alloc() {
                self.p2[p2_index].set(frame, F::VALID);
                self.edit_p1(p2_index, |p1| p1.zero());
            } else {
                return Err(MapToError::FrameAllocationFailed);
            }
        }
        Ok(())
    }

    /// Edit a p1 page.
    /// During the editing, the flag of entry `p2[p2_index]` is temporarily set to V+R+W.
    fn edit_p1<F, T>(&mut self, p2_index: usize, f: F) -> T where F: FnOnce(&mut PageTable) -> T {
        type F = PageTableFlags;
        let flags = self.p2[p2_index].flags_mut();
        assert_ne!(p2_index, self.recursive_index, "can not edit recursive index");
        assert_ne!(p2_index, self.recursive_index + 1, "can not edit recursive index");
        assert!(flags.contains(F::VALID), "try to edit a nonexistent p1 table");
        assert!(!flags.contains(F::READABLE) && !flags.contains(F::WRITABLE), "try to edit a 4M page as p1 table");
        flags.insert(F::READABLE | F::WRITABLE);
        let p1 = Page::from_page_table_indices(self.recursive_index, p2_index);
        let p1 = unsafe{ &mut *(p1.start_address().as_usize() as *mut PageTable) };
        let ret = f(p1);
        flags.remove(F::READABLE | F::WRITABLE);
        ret
    }
}

impl<'a> Mapper for RecursivePageTable<'a> {
    fn map_to<A>(&mut self, page: Page, frame: Frame, flags: PageTableFlags, allocator: &mut A) -> Result<MapperFlush, MapToError>
        where A: FrameAllocator,
    {
        //use PageTableFlags as Flags;
        self.create_p1_if_not_exist(page.p2_index(), allocator)?;
        self.edit_p1(page.p2_index(), |p1| {
            if !p1[page.p1_index()].is_unused() {
                return Err(MapToError::PageAlreadyMapped);
            }
            p1[page.p1_index()].set(frame, flags);
            Ok(MapperFlush::new(page))
        })
    }

    fn unmap(&mut self, page: Page) -> Result<(Frame, MapperFlush), UnmapError> {
        //use PageTableFlags as Flags;
        if self.p2[page.p2_index()].is_unused() {
            return Err(UnmapError::PageNotMapped);
        }
        self.edit_p1(page.p2_index(), |p1| {
            let p1_entry = &mut p1[page.p1_index()];
            if !p1_entry.flags().contains(PageTableFlags::VALID) {
                return Err(UnmapError::PageNotMapped);
            }
            let frame = p1_entry.frame();
            p1_entry.set_unused();
            Ok((frame, MapperFlush::new(page)))
        })
    }

    fn translate_page(&self, page: Page) -> Option<Frame> {
        if self.p2[page.p2_index()].is_unused() {
            return None;
        }
        let self_mut = unsafe{ &mut *(self as *const _ as *mut Self) };
        self_mut.edit_p1(page.p2_index(), |p1| {
            let p1_entry = &p1[page.p1_index()];
            if p1_entry.is_unused() {
                return None;
            }
            Some(p1_entry.frame())
        })
    }
}
