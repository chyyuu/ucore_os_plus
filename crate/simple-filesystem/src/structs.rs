//! On-disk structures in SFS

use core::slice;
use core::mem::{size_of_val, size_of};
use core::fmt::{Debug, Formatter, Error};
use alloc::str;

/// On-disk superblock
#[repr(C, packed)]
#[derive(Debug)]
pub struct SuperBlock {
    /// magic number, should be SFS_MAGIC
    pub magic: u32,
    /// number of blocks in fs
    pub blocks: u32,
    /// number of unused blocks in fs
    pub unused_blocks: u32,
    /// information for sfs
    pub info: Str32,
}

/// inode (on disk)
#[repr(C, packed)]
#[derive(Debug)]
pub struct DiskINode {
    /// size of the file (in bytes)
    /// undefined in dir (256 * #entries ?)
    pub size: u32,
    /// one of SYS_TYPE_* above
    pub type_: FileType,
    /// number of hard links to this file
    /// Note: "." and ".." is counted in this nlinks
    pub nlinks: u16,
    /// number of blocks
    pub blocks: u32,
    /// direct blocks
    pub direct: [u32; NDIRECT],
    /// indirect blocks
    pub indirect: u32,
    /// double indirect blocks
    pub db_indirect: u32,
}

#[repr(C, packed)]
pub struct IndirectBlock {
    pub entries: [u32; BLK_NENTRY],
}

/// file entry (on disk)
#[repr(C, packed)]
#[derive(Debug)]
pub struct DiskEntry {
    /// inode number
    pub id: u32,
    /// file name
    pub name: Str256,
}

#[repr(C)]
pub struct Str256(pub [u8; 256]);

#[repr(C)]
pub struct Str32(pub [u8; 32]);

impl AsRef<str> for Str256 {
    fn as_ref(&self) -> &str {
        let len = self.0.iter().enumerate().find(|(_, &b)| b == 0).unwrap().0;
        str::from_utf8(&self.0[0..len]).unwrap()
    }
}

impl AsRef<str> for Str32 {
    fn as_ref(&self) -> &str {
        let len = self.0.iter().enumerate().find(|(_, &b)| b == 0).unwrap().0;
        str::from_utf8(&self.0[0..len]).unwrap()
    }
}

impl Debug for Str256 {
    fn fmt(&self, f: &mut Formatter) -> Result<(), Error> {
        write!(f, "{}", self.as_ref())
    }
}

impl Debug for Str32 {
    fn fmt(&self, f: &mut Formatter) -> Result<(), Error> {
        write!(f, "{}", self.as_ref())
    }
}

impl<'a> From<&'a str> for Str256 {
    fn from(s: &'a str) -> Self {
        let mut ret = [0u8; 256];
        ret[0..s.len()].copy_from_slice(s.as_ref());
        Str256(ret)
    }
}

impl<'a> From<&'a str> for Str32 {
    fn from(s: &'a str) -> Self {
        let mut ret = [0u8; 32];
        ret[0..s.len()].copy_from_slice(s.as_ref());
        Str32(ret)
    }
}

impl SuperBlock {
    pub fn check(&self) -> bool {
        self.magic == MAGIC
    }
}

impl DiskINode {
    pub const fn new_file() -> Self {
        DiskINode {
            size: 0,
            type_: FileType::File,
            nlinks: 0,
            blocks: 0,
            direct: [0; NDIRECT],
            indirect: 0,
            db_indirect: 0,
        }
    }
    pub const fn new_dir() -> Self {
        DiskINode {
            size: 0,
            type_: FileType::Dir,
            nlinks: 0,
            blocks: 0,
            direct: [0; NDIRECT],
            indirect: 0,
            db_indirect: 0,
        }
    }
}

/// Convert structs to [u8] slice
pub trait AsBuf {
    fn as_buf(&self) -> &[u8] {
        unsafe { slice::from_raw_parts(self as *const _ as *const u8, size_of_val(self)) }
    }
    fn as_buf_mut(&mut self) -> &mut [u8] {
        unsafe { slice::from_raw_parts_mut(self as *mut _ as *mut u8, size_of_val(self)) }
    }
}

impl AsBuf for SuperBlock {}

impl AsBuf for DiskINode {}

impl AsBuf for DiskEntry {}

impl AsBuf for u32 {}

/*
 * Simple FS (SFS) definitions visible to ucore. This covers the on-disk format
 * and is used by tools that work on SFS volumes, such as mksfs.
 */
pub type BlockId = usize;
pub type INodeId = BlockId;

/// magic number for sfs
pub const MAGIC: u32 = 0x2f8dbe2a;
/// size of block
pub const BLKSIZE: usize = 1usize << BLKSIZE_LOG2;
/// log2( size of block )
pub const BLKSIZE_LOG2: u8 = 12;
/// number of direct blocks in inode
pub const NDIRECT: usize = 12;
/// max length of infomation
pub const MAX_INFO_LEN: usize = 31;
/// max length of filename
pub const MAX_FNAME_LEN: usize = 255;
/// max file size (128M)
pub const MAX_FILE_SIZE: usize = 1024 * 1024 * 128;
/// block the superblock lives in
pub const BLKN_SUPER: BlockId = 0;
/// location of the root dir inode
pub const BLKN_ROOT: BlockId = 1;
/// 1st block of the freemap
pub const BLKN_FREEMAP: BlockId = 2;
/// number of bits in a block
pub const BLKBITS: usize = BLKSIZE * 8;
///
pub const ENTRY_SIZE: usize = 4;
/// number of entries in a block
pub const BLK_NENTRY: usize = BLKSIZE / ENTRY_SIZE;
/// size of a dirent used in the size field
pub const DIRENT_SIZE: usize = MAX_FNAME_LEN + 1;

/// file types
#[repr(u16)]
#[derive(Debug, Eq, PartialEq, Copy, Clone)]
pub enum FileType {
    Invalid = 0,
    File = 1,
    Dir = 2,
    Link = 3,
}

const_assert!(o1; size_of::<SuperBlock>() <= BLKSIZE);
const_assert!(o2; size_of::<DiskINode>() <= BLKSIZE);
const_assert!(o3; size_of::<DiskEntry>() <= BLKSIZE);
const_assert!(o4; size_of::<IndirectBlock>() == BLKSIZE);
