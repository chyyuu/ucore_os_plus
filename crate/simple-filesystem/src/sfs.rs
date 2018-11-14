use bit_vec::BitVec;
use alloc::{boxed::Box, vec::Vec, collections::BTreeMap, rc::{Rc, Weak}, string::String};
use core::cell::{RefCell, RefMut};
use core::mem::{uninitialized, size_of};
use core::slice;
use core::fmt::{Debug, Formatter, Error};
use core::any::Any;
use dirty::Dirty;
use structs::*;
use vfs::{self, Device};
use util::*;

impl Device {
    fn read_block(&mut self, id: BlockId, offset: usize, buf: &mut [u8]) -> vfs::Result<()> {
        debug_assert!(offset + buf.len() <= BLKSIZE);
        match self.read_at(id * BLKSIZE + offset, buf) {
            Some(len) if len == buf.len() => Ok(()),
            _ => Err(()),
        }
    }
    fn write_block(&mut self, id: BlockId, offset: usize, buf: &[u8]) -> vfs::Result<()> {
        debug_assert!(offset + buf.len() <= BLKSIZE);
        match self.write_at(id * BLKSIZE + offset, buf) {
            Some(len) if len == buf.len() => Ok(()),
            _ => Err(()),
        }
    }
    /// Load struct `T` from given block in device
    fn load_struct<T: AsBuf>(&mut self, id: BlockId) -> T {
        let mut s: T = unsafe { uninitialized() };
        self.read_block(id, 0, s.as_buf_mut()).unwrap();
        s
    }
}

type Ptr<T> = Rc<RefCell<T>>;

/// inode for sfs
pub struct INode {
    /// on-disk inode
    disk_inode: Dirty<DiskINode>,
    /// inode number
    id: INodeId,
    /// Weak reference to SFS, used by almost all operations
    fs: Weak<SimpleFileSystem>,
}

impl Debug for INode {
    fn fmt(&self, f: &mut Formatter) -> Result<(), Error> {
        write!(f, "INode {{ id: {}, disk: {:?} }}", self.id, self.disk_inode)
    }
}

impl INode {
    /// Map file block id to disk block id
    fn get_disk_block_id(&self, file_block_id: BlockId) -> Option<BlockId> {
        match file_block_id {
            id if id >= self.disk_inode.blocks as BlockId =>
                None,
            id if id < NDIRECT =>
                Some(self.disk_inode.direct[id] as BlockId),
            id if id < NDIRECT + BLK_NENTRY => {
                let mut disk_block_id: u32 = 0;
                let fs = self.fs.upgrade().unwrap();
                fs.device.borrow_mut().read_block(
                    self.disk_inode.indirect as usize,
                    ENTRY_SIZE * (id - NDIRECT),
                    disk_block_id.as_buf_mut(),
                ).unwrap();
                Some(disk_block_id as BlockId)
            }
            _ => unimplemented!("double indirect blocks is not supported"),
        }
    }
    fn set_disk_block_id(&mut self, file_block_id: BlockId, disk_block_id: BlockId) -> vfs::Result<()> {
        match file_block_id {
            id if id >= self.disk_inode.blocks as BlockId =>
                Err(()),
            id if id < NDIRECT => {
                self.disk_inode.direct[id] = disk_block_id as u32;
                Ok(())
            }
            id if id < NDIRECT + BLK_NENTRY => {
                let disk_block_id = disk_block_id as u32;
                let fs = self.fs.upgrade().unwrap();
                fs.device.borrow_mut().write_block(
                    self.disk_inode.indirect as usize,
                    ENTRY_SIZE * (id - NDIRECT),
                    disk_block_id.as_buf(),
                ).unwrap();
                Ok(())
            }
            _ => unimplemented!("double indirect blocks is not supported"),
        }
    }
    /// Only for Dir
    fn get_file_inode_and_entry_id(&self, name: &str) -> Option<(INodeId, usize)> {
        (0..self.disk_inode.blocks)
            .map(|i| {
                use vfs::INode;
                let mut entry: DiskEntry = unsafe { uninitialized() };
                self._read_at(i as usize * BLKSIZE, entry.as_buf_mut()).unwrap();
                (entry, i)
            })
            .find(|(entry, id)| entry.name.as_ref() == name)
            .map(|(entry, id)| (entry.id as INodeId, id as usize))
    }
    fn get_file_inode_id(&self, name: &str) -> Option<INodeId> {
        self.get_file_inode_and_entry_id(name).map(|(inode_id, entry_id)| { inode_id })
    }
    /// Init dir content. Insert 2 init entries.
    /// This do not init nlinks, please modify the nlinks in the invoker.
    fn init_dir_entry(&mut self, parent: INodeId) -> vfs::Result<()> {
        use vfs::INode;
        let fs = self.fs.upgrade().unwrap();
        // Insert entries: '.' '..'
        self._resize(BLKSIZE * 2).unwrap();
        self._write_at(BLKSIZE * 1, DiskEntry {
            id: parent as u32,
            name: Str256::from(".."),
        }.as_buf()).unwrap();
        self._write_at(BLKSIZE * 0, DiskEntry {
            id: self.id as u32,
            name: Str256::from("."),
        }.as_buf()).unwrap();
        Ok(())
    }
    /// remove a page in middle of file and insert the last page here, useful for dirent remove
    /// should be only used in unlink
    fn remove_dirent_page(&mut self, id: usize) -> vfs::Result<()> {
        assert!(id < self.disk_inode.blocks as usize);
        let fs = self.fs.upgrade().unwrap();
        let to_remove = self.get_disk_block_id(id).unwrap();
        let current_last = self.get_disk_block_id(self.disk_inode.blocks as usize - 1).unwrap();
        self.set_disk_block_id(id, current_last).unwrap();
        self.disk_inode.blocks -= 1;
        let new_size = self.disk_inode.blocks as usize * BLKSIZE;
        self._set_size(new_size);
        fs.free_block(to_remove);
        Ok(())
    }
    /// Resize content size, no matter what type it is.
    fn _resize(&mut self, len: usize) -> vfs::Result<()> {
        assert!(len <= MAX_FILE_SIZE, "file size exceed limit");
        let blocks = ((len + BLKSIZE - 1) / BLKSIZE) as u32;
        use core::cmp::{Ord, Ordering};
        match blocks.cmp(&self.disk_inode.blocks) {
            Ordering::Equal => {}  // Do nothing
            Ordering::Greater => {
                let fs = self.fs.upgrade().unwrap();
                let old_blocks = self.disk_inode.blocks;
                self.disk_inode.blocks = blocks;
                // allocate indirect block if need
                if old_blocks < NDIRECT as u32 && blocks >= NDIRECT as u32 {
                    self.disk_inode.indirect = fs.alloc_block().unwrap() as u32;
                }
                // allocate extra blocks
                for i in old_blocks..blocks {
                    let disk_block_id = fs.alloc_block().expect("no more space");
                    self.set_disk_block_id(i as usize, disk_block_id).unwrap();
                }
                // clean up
                let old_size = self._size();
                self._set_size(len);
                self._clean_at(old_size, len).unwrap();
            }
            Ordering::Less => {
                let fs = self.fs.upgrade().unwrap();
                // free extra blocks
                for i in blocks..self.disk_inode.blocks {
                    let disk_block_id = self.get_disk_block_id(i as usize).unwrap();
                    fs.free_block(disk_block_id);
                }
                // free indirect block if need
                if blocks < NDIRECT as u32 && self.disk_inode.blocks >= NDIRECT as u32 {
                    fs.free_block(self.disk_inode.indirect as usize);
                    self.disk_inode.indirect = 0;
                }
                self.disk_inode.blocks = blocks;
            }
        }
        self._set_size(len);
        Ok(())
    }
    /// Get the actual size of this inode,
    /// since size in inode for dir is not real size
    fn _size(&self) -> usize {
        match self.disk_inode.type_ {
            FileType::Dir => self.disk_inode.blocks as usize * BLKSIZE,
            FileType::File => self.disk_inode.size as usize,
            _ => unimplemented!(),
        }
    }
    /// Set the ucore compat size of this inode,
    /// Size in inode for dir is size of entries
    fn _set_size(&mut self, len: usize) {
        self.disk_inode.size = match self.disk_inode.type_ {
            FileType::Dir => self.disk_inode.blocks as usize * DIRENT_SIZE,
            FileType::File => len,
            _ => unimplemented!(),
        } as u32
    }
    /// Read/Write content, no matter what type it is
    fn _io_at<F>(&self, begin: usize, end: usize, mut f: F) -> vfs::Result<usize>
        where F: FnMut(RefMut<Box<Device>>, &BlockRange, usize)
    {
        let fs = self.fs.upgrade().unwrap();

        let size = self._size();
        let iter = BlockIter {
            begin: size.min(begin),
            end: size.min(end),
            block_size_log2: BLKSIZE_LOG2,
        };

        // For each block
        let mut buf_offset = 0usize;
        for mut range in iter {
            range.block = self.get_disk_block_id(range.block).unwrap();
            f(fs.device.borrow_mut(), &range, buf_offset);
            buf_offset += range.len();
        }
        Ok(buf_offset)
    }
    /// Read content, no matter what type it is
    fn _read_at(&self, offset: usize, buf: &mut [u8]) -> vfs::Result<usize> {
        self._io_at(offset, offset + buf.len(), |mut device, range, offset| {
            device.read_block(range.block, range.begin, &mut buf[offset..offset + range.len()]).unwrap()
        })
    }
    /// Write content, no matter what type it is
    fn _write_at(&self, offset: usize, buf: &[u8]) -> vfs::Result<usize> {
        self._io_at(offset, offset + buf.len(), |mut device, range, offset| {
            device.write_block(range.block, range.begin, &buf[offset..offset + range.len()]).unwrap()
        })
    }
    /// Clean content, no matter what type it is
    fn _clean_at(&self, begin: usize, end: usize) -> vfs::Result<()> {
        static ZEROS: [u8; BLKSIZE] = [0; BLKSIZE];
        self._io_at(begin, end, |mut device, range, _| {
            device.write_block(range.block, range.begin, &ZEROS[..range.len()]).unwrap()
        }).unwrap();
        Ok(())
    }
    fn nlinks_inc(&mut self) {
        self.disk_inode.nlinks += 1;
    }
    fn nlinks_dec(&mut self) {
        assert!(self.disk_inode.nlinks > 0);
        self.disk_inode.nlinks -= 1;
    }
}

impl vfs::INode for INode {
    fn open(&mut self, flags: u32) -> vfs::Result<()> {
        // Do nothing
        Ok(())
    }
    fn close(&mut self) -> vfs::Result<()> {
        self.sync()
    }
    fn read_at(&self, offset: usize, buf: &mut [u8]) -> vfs::Result<usize> {
        assert_eq!(self.disk_inode.type_, FileType::File, "read_at is only available on file");
        self._read_at(offset, buf)
    }
    fn write_at(&self, offset: usize, buf: &[u8]) -> vfs::Result<usize> {
        assert_eq!(self.disk_inode.type_, FileType::File, "write_at is only available on file");
        self._write_at(offset, buf)
    }
    /// the size returned here is logical size(entry num for directory), not the disk space used.
    fn info(&self) -> vfs::Result<vfs::FileInfo> {
        Ok(vfs::FileInfo {
            size: match self.disk_inode.type_ {
                FileType::File => self.disk_inode.size as usize,
                FileType::Dir => self.disk_inode.blocks as usize,
                _ => unimplemented!(),
            },
            mode: 0,
            type_: vfs::FileType::from(self.disk_inode.type_.clone()),
            blocks: self.disk_inode.blocks as usize,
            nlinks: self.disk_inode.nlinks as usize,
        })
    }
    fn sync(&mut self) -> vfs::Result<()> {
        if self.disk_inode.dirty() {
            let fs = self.fs.upgrade().unwrap();
            fs.device.borrow_mut().write_block(self.id, 0, self.disk_inode.as_buf()).unwrap();
            self.disk_inode.sync();
        }
        Ok(())
    }
    fn resize(&mut self, len: usize) -> vfs::Result<()> {
        assert_eq!(self.disk_inode.type_, FileType::File, "resize is only available on file");
        self._resize(len)
    }
    fn create(&mut self, name: &str, type_: vfs::FileType) -> vfs::Result<Ptr<vfs::INode>> {
        let fs = self.fs.upgrade().unwrap();
        let info = self.info().unwrap();
        assert_eq!(info.type_, vfs::FileType::Dir);
        assert!(info.nlinks > 0);

        // Ensure the name is not exist
        assert!(self.get_file_inode_id(name).is_none(), "file name exist");

        // Create new INode
        let inode = match type_ {
            vfs::FileType::File => fs.new_inode_file().unwrap(),
            vfs::FileType::Dir => fs.new_inode_dir(self.id).unwrap(),
        };

        // Write new entry
        let entry = DiskEntry {
            id: inode.borrow().id as u32,
            name: Str256::from(name),
        };
        let old_size = self._size();
        self._resize(old_size + BLKSIZE).unwrap();
        self._write_at(old_size, entry.as_buf()).unwrap();
        inode.borrow_mut().nlinks_inc();
        if type_ == vfs::FileType::Dir {
            inode.borrow_mut().nlinks_inc();//for .
            self.nlinks_inc();//for ..
        }

        Ok(inode)
    }
    fn unlink(&mut self, name: &str) -> vfs::Result<()> {
        assert!(name != ".");
        assert!(name != "..");
        let fs = self.fs.upgrade().unwrap();
        let info = self.info().unwrap();
        assert_eq!(info.type_, vfs::FileType::Dir);

        let inode_and_entry_id = self.get_file_inode_and_entry_id(name);
        if inode_and_entry_id.is_none() {
            return Err(());
        }
        let (inode_id, entry_id) = inode_and_entry_id.unwrap();
        let inode = fs.get_inode(inode_id);

        let type_ = inode.borrow().disk_inode.type_;
        if type_ == FileType::Dir {
            // only . and ..
            assert_eq!(inode.borrow().disk_inode.blocks, 2);
        }
        inode.borrow_mut().nlinks_dec();
        if type_ == FileType::Dir {
            inode.borrow_mut().nlinks_dec();//for .
            self.nlinks_dec();//for ..
        }
        self.remove_dirent_page(entry_id);

        Ok(())
    }
    fn link(&mut self, name: &str, other: &mut vfs::INode) -> vfs::Result<()> {
        let fs = self.fs.upgrade().unwrap();
        let info = self.info().unwrap();
        assert_eq!(info.type_, vfs::FileType::Dir);
        assert!(info.nlinks > 0);
        assert!(self.get_file_inode_id(name).is_none(), "file name exist");
        let child = other.downcast_mut::<INode>().unwrap();
        assert!(Rc::ptr_eq(&fs, &child.fs.upgrade().unwrap()));
        assert!(child.info().unwrap().type_ != vfs::FileType::Dir);
        let entry = DiskEntry {
            id: child.id as u32,
            name: Str256::from(name),
        };
        let old_size = self._size();
        self._resize(old_size + BLKSIZE).unwrap();
        self._write_at(old_size, entry.as_buf()).unwrap();
        child.nlinks_inc();
        Ok(())
    }
    fn rename(&mut self, old_name: &str, new_name: &str) -> vfs::Result<()> {
        let info = self.info().unwrap();
        assert_eq!(info.type_, vfs::FileType::Dir);
        assert!(info.nlinks > 0);

        assert!(self.get_file_inode_id(new_name).is_none(), "file name exist");

        let inode_and_entry_id = self.get_file_inode_and_entry_id(old_name);
        if inode_and_entry_id.is_none() {
            return Err(());
        }
        let (_, entry_id) = inode_and_entry_id.unwrap();

        // in place modify name
        let mut entry: DiskEntry = unsafe { uninitialized() };
        let entry_pos = entry_id as usize * BLKSIZE;
        self._read_at(entry_pos, entry.as_buf_mut()).unwrap();
        entry.name = Str256::from(new_name);
        self._write_at(entry_pos, entry.as_buf()).unwrap();

        Ok(())
    }
    fn move_(&mut self, old_name: &str, target: &mut vfs::INode, new_name: &str) -> vfs::Result<()> {
        let fs = self.fs.upgrade().unwrap();
        let info = self.info().unwrap();
        assert_eq!(info.type_, vfs::FileType::Dir);
        assert!(info.nlinks > 0);
        let dest = target.downcast_mut::<INode>().unwrap();
        assert!(Rc::ptr_eq(&fs, &dest.fs.upgrade().unwrap()));
        assert!(dest.info().unwrap().type_ == vfs::FileType::Dir);
        assert!(dest.info().unwrap().nlinks > 0);

        assert!(dest.get_file_inode_id(new_name).is_none(), "file name exist");

        let inode_and_entry_id = self.get_file_inode_and_entry_id(old_name);
        if inode_and_entry_id.is_none() {
            return Err(());
        }
        let (inode_id, entry_id) = inode_and_entry_id.unwrap();
        let inode = fs.get_inode(inode_id);

        let entry = DiskEntry {
            id: inode_id as u32,
            name: Str256::from(new_name),
        };
        let old_size = dest._size();
        dest._resize(old_size + BLKSIZE).unwrap();
        dest._write_at(old_size, entry.as_buf()).unwrap();

        self.remove_dirent_page(entry_id);

        if inode.borrow().info().unwrap().type_ == vfs::FileType::Dir {
            self.nlinks_dec();
            dest.nlinks_inc();
        }

        Ok(())
    }
    fn find(&self, name: &str) -> vfs::Result<Ptr<vfs::INode>> {
        let fs = self.fs.upgrade().unwrap();
        let info = self.info().unwrap();
        assert_eq!(info.type_, vfs::FileType::Dir);
        let inode_id = self.get_file_inode_id(name);
        if inode_id.is_none() {
            return Err(());
        }
        let inode = fs.get_inode(inode_id.unwrap());
        Ok(inode)
    }
    fn get_entry(&self, id: usize) -> vfs::Result<String> {
        assert_eq!(self.disk_inode.type_, FileType::Dir);
        assert!(id < self.disk_inode.blocks as usize);
        use vfs::INode;
        let mut entry: DiskEntry = unsafe { uninitialized() };
        self._read_at(id as usize * BLKSIZE, entry.as_buf_mut()).unwrap();
        Ok(String::from(entry.name.as_ref()))
    }
    fn fs(&self) -> Weak<vfs::FileSystem> {
        self.fs.clone()
    }
    fn as_any_ref(&self) -> &Any {
        self
    }
    fn as_any_mut(&mut self) -> &mut Any {
        self
    }
}

impl Drop for INode {
    /// Auto sync when drop
    fn drop(&mut self) {
        use vfs::INode;
        self.sync().expect("failed to sync");
        if self.disk_inode.nlinks <= 0 {
            let fs = self.fs.upgrade().unwrap();
            self._resize(0);
            self.disk_inode.sync();
            fs.free_block(self.id);
        }
    }
}


/// filesystem for sfs
///
/// ## 内部可变性
/// 为了方便协调外部及INode对SFS的访问，并为日后并行化做准备，
/// 将SFS设置为内部可变，即对外接口全部是&self，struct的全部field用RefCell包起来
/// 这样其内部各field均可独立访问
pub struct SimpleFileSystem {
    /// on-disk superblock
    super_block: RefCell<Dirty<SuperBlock>>,
    /// blocks in use are mared 0
    free_map: RefCell<Dirty<BitVec>>,
    /// inode list
    inodes: RefCell<BTreeMap<INodeId, Ptr<INode>>>,
    /// device
    device: RefCell<Box<Device>>,
    /// Pointer to self, used by INodes
    self_ptr: Weak<SimpleFileSystem>,
}

impl SimpleFileSystem {
    /// Load SFS from device
    pub fn open(mut device: Box<Device>) -> Option<Rc<Self>> {
        let super_block = device.load_struct::<SuperBlock>(BLKN_SUPER);
        if super_block.check() == false {
            return None;
        }
        let free_map = device.load_struct::<[u8; BLKSIZE]>(BLKN_FREEMAP);

        Some(SimpleFileSystem {
            super_block: RefCell::new(Dirty::new(super_block)),
            free_map: RefCell::new(Dirty::new(BitVec::from_bytes(&free_map))),
            inodes: RefCell::new(BTreeMap::<INodeId, Ptr<INode>>::new()),
            device: RefCell::new(device),
            self_ptr: Weak::default(),
        }.wrap())
    }
    /// Create a new SFS on blank disk
    pub fn create(mut device: Box<Device>, space: usize) -> Rc<Self> {
        let blocks = (space / BLKSIZE).min(BLKBITS);
        assert!(blocks >= 16, "space too small");

        let super_block = SuperBlock {
            magic: MAGIC,
            blocks: blocks as u32,
            unused_blocks: blocks as u32 - 3,
            info: Str32::from("simple file system"),
        };
        let free_map = {
            let mut bitset = BitVec::from_elem(BLKBITS, false);
            for i in 3..blocks {
                bitset.set(i, true);
            }
            bitset
        };

        let sfs = SimpleFileSystem {
            super_block: RefCell::new(Dirty::new_dirty(super_block)),
            free_map: RefCell::new(Dirty::new_dirty(free_map)),
            inodes: RefCell::new(BTreeMap::new()),
            device: RefCell::new(device),
            self_ptr: Weak::default(),
        }.wrap();

        // Init root INode
        use vfs::INode;
        let root = sfs._new_inode(BLKN_ROOT, Dirty::new_dirty(DiskINode::new_dir()));
        root.borrow_mut().init_dir_entry(BLKN_ROOT).unwrap();
        root.borrow_mut().nlinks_inc();//for .
        root.borrow_mut().nlinks_inc();//for ..(root's parent is itself)
        root.borrow_mut().sync().unwrap();

        sfs
    }
    /// Wrap pure SimpleFileSystem with Rc
    /// Used in constructors
    fn wrap(self) -> Rc<Self> {
        // Create a Rc, make a Weak from it, then put it into the struct.
        // It's a little tricky.
        let fs = Rc::new(self);
        let weak = Rc::downgrade(&fs);
        let ptr = Rc::into_raw(fs) as *mut Self;
        unsafe { (*ptr).self_ptr = weak; }
        unsafe { Rc::from_raw(ptr) }
    }

    /// Allocate a block, return block id
    fn alloc_block(&self) -> Option<usize> {
        let id = self.free_map.borrow_mut().alloc();
        if id.is_some() {
            self.super_block.borrow_mut().unused_blocks -= 1;    // will panic if underflow
            id
        } else {
            self.flush_unreachable_inodes();
            let id = self.free_map.borrow_mut().alloc();
            if id.is_some() {
                self.super_block.borrow_mut().unused_blocks -= 1;
            }
            id
        }
    }
    /// Free a block
    fn free_block(&self, block_id: usize) {
        let mut free_map = self.free_map.borrow_mut();
        assert!(!free_map[block_id]);
        free_map.set(block_id, true);
        self.super_block.borrow_mut().unused_blocks += 1;
    }

    /// Create a new INode struct, then insert it to self.inodes
    /// Private used for load or create INode
    fn _new_inode(&self, id: INodeId, disk_inode: Dirty<DiskINode>) -> Ptr<INode> {
        let inode = Rc::new(RefCell::new(INode {
            disk_inode,
            id,
            fs: self.self_ptr.clone(),
        }));
        self.inodes.borrow_mut().insert(id, inode.clone());
        inode
    }
    /// Get inode by id. Load if not in memory.
    /// ** Must ensure it's a valid INode **
    fn get_inode(&self, id: INodeId) -> Ptr<INode> {
        assert!(!self.free_map.borrow()[id]);

        // Load if not in memory.
        if !self.inodes.borrow().contains_key(&id) {
            let disk_inode = Dirty::new(self.device.borrow_mut().load_struct::<DiskINode>(id));
            self._new_inode(id, disk_inode)
        } else {
            self.inodes.borrow_mut().get(&id).unwrap().clone()
        }
    }
    /// Create a new INode file
    fn new_inode_file(&self) -> vfs::Result<Ptr<INode>> {
        let id = self.alloc_block().unwrap();
        let disk_inode = Dirty::new_dirty(DiskINode::new_file());
        Ok(self._new_inode(id, disk_inode))
    }
    /// Create a new INode dir
    fn new_inode_dir(&self, parent: INodeId) -> vfs::Result<Ptr<INode>> {
        let id = self.alloc_block().unwrap();
        let disk_inode = Dirty::new_dirty(DiskINode::new_dir());
        let inode = self._new_inode(id, disk_inode);
        inode.borrow_mut().init_dir_entry(parent).unwrap();
        Ok(inode)
    }
    fn flush_unreachable_inodes(&self) {
        let mut inodes = self.inodes.borrow_mut();
        let remove_ids: Vec<_> = inodes.iter().filter(|(_, inode)| {
            use vfs::INode;
            Rc::strong_count(inode) <= 1
                && inode.borrow().info().unwrap().nlinks == 0
        }).map(|(&id, _)| id).collect();
        for id in remove_ids.iter() {
            inodes.remove(&id);
        }
    }
}

impl vfs::FileSystem for SimpleFileSystem {
    /// Write back super block if dirty
    fn sync(&self) -> vfs::Result<()> {
        let mut super_block = self.super_block.borrow_mut();
        if super_block.dirty() {
            self.device.borrow_mut().write_at(BLKSIZE * BLKN_SUPER, super_block.as_buf()).unwrap();
            super_block.sync();
        }
        let mut free_map = self.free_map.borrow_mut();
        if free_map.dirty() {
            self.device.borrow_mut().write_at(BLKSIZE * BLKN_FREEMAP, free_map.as_buf()).unwrap();
            free_map.sync();
        }
        for inode in self.inodes.borrow().values() {
            use vfs::INode;
            inode.borrow_mut().sync().unwrap();
        }
        self.flush_unreachable_inodes();
        Ok(())
    }

    fn root_inode(&self) -> Ptr<vfs::INode> {
        self.get_inode(BLKN_ROOT)
    }

    fn info(&self) -> &'static vfs::FsInfo {
        static INFO: vfs::FsInfo = vfs::FsInfo {
            max_file_size: MAX_FILE_SIZE,
        };
        &INFO
    }
}

impl Drop for SimpleFileSystem {
    /// Auto sync when drop
    fn drop(&mut self) {
        use vfs::FileSystem;
        self.sync().expect("failed to sync");
    }
}

trait BitsetAlloc {
    fn alloc(&mut self) -> Option<usize>;
}

impl BitsetAlloc for BitVec {
    fn alloc(&mut self) -> Option<usize> {
        // TODO: more efficient
        let id = (0..self.len()).find(|&i| self[i]);
        if let Some(id) = id {
            self.set(id, false);
        }
        id
    }
}

impl AsBuf for BitVec {
    fn as_buf(&self) -> &[u8] {
        let slice = self.storage();
        unsafe { slice::from_raw_parts(slice as *const _ as *const u8, slice.len() * 4) }
    }
    fn as_buf_mut(&mut self) -> &mut [u8] {
        let slice = self.storage();
        unsafe { slice::from_raw_parts_mut(slice as *const _ as *mut u8, slice.len() * 4) }
    }
}

impl AsBuf for [u8; BLKSIZE] {}

impl From<FileType> for vfs::FileType {
    fn from(t: FileType) -> Self {
        match t {
            FileType::File => vfs::FileType::File,
            FileType::Dir => vfs::FileType::Dir,
            _ => panic!("unknown file type"),
        }
    }
}
