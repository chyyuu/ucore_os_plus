use std::fs;
use std::fs::{File, OpenOptions};
use std::io::{Read, Write, Seek, SeekFrom};
use std::boxed::Box;
use super::sfs::*;
use super::vfs::*;
use super::vfs::INode;
use std::rc::Rc;
use std::mem::uninitialized;
use super::structs::{DiskEntry, AsBuf};

fn _open_sample_file() -> Rc<SimpleFileSystem> {
    fs::copy("sfs.img", "test.img").expect("failed to open sfs.img");
    let file = OpenOptions::new()
        .read(true).write(true).open("test.img")
        .expect("failed to open test.img");
    SimpleFileSystem::open(Box::new(file))
        .expect("failed to open SFS")
}

fn _create_new_sfs() -> Rc<SimpleFileSystem> {
    let file = OpenOptions::new()
        .read(true).write(true).create(true).open("test.img")
        .expect("failed to create file");
    SimpleFileSystem::create(Box::new(file), 32 * 4096)
}

//#[test]
fn open_sample_file() {
    _open_sample_file();
}

#[test]
fn create_new_sfs() {
    let sfs = _create_new_sfs();
    let root = sfs.root_inode();
}

// #[test]
fn print_root() {
    let sfs = _open_sample_file();
    let root = sfs.root_inode();
    println!("{:?}", root.borrow());

    let files = root.borrow().list().unwrap();
    println!("{:?}", files);
    assert_eq!(files[3], root.borrow().get_entry(3).unwrap());

    sfs.sync().unwrap();
}

#[test]
fn create_file() {
    let sfs = _create_new_sfs();
    let root = sfs.root_inode();
    let file1 = root.borrow_mut().create("file1", FileType::File).unwrap();

    assert_eq!(file1.borrow().info().unwrap(), FileInfo {
        size: 0,
        type_: FileType::File,
        mode: 0,
        blocks: 0,
        nlinks: 1,
    });

    sfs.sync().unwrap();
}

#[test]
fn resize() {
    let sfs = _create_new_sfs();
    let root = sfs.root_inode();
    let file1 = root.borrow_mut().create("file1", FileType::File).unwrap();
    assert_eq!(file1.borrow().info().unwrap().size, 0, "empty file size != 0");

    const SIZE1: usize = 0x1234;
    const SIZE2: usize = 0x1250;
    file1.borrow_mut().resize(SIZE1).unwrap();
    assert_eq!(file1.borrow().info().unwrap().size, SIZE1, "wrong size after resize");
    let mut data1: [u8; SIZE2] = unsafe { uninitialized() };
    impl AsBuf for [u8; SIZE2] {}
    let len = file1.borrow().read_at(0, data1.as_buf_mut()).unwrap();
    assert_eq!(len, SIZE1, "wrong size returned by read_at()");
    assert_eq!(&data1[..SIZE1], &[0u8; SIZE1][..], "expanded data should be 0");

    sfs.sync().unwrap();
}

// FIXME: `should_panic` tests will panic again on exit, due to `Dirty` drop

//#[test]
//#[should_panic]
//fn resize_on_dir_should_panic() {
//    let sfs = _create_new_sfs();
//    let root = sfs.root_inode();
//    root.borrow_mut().resize(4096).unwrap();
//    sfs.sync().unwrap();
//}
//
//#[test]
//#[should_panic]
//fn resize_too_large_should_panic() {
//    let sfs = _create_new_sfs();
//    let root = sfs.root_inode();
//    let file1 = root.borrow_mut().create("file1", FileType::File).unwrap();
//    file1.borrow_mut().resize(1 << 28).unwrap();
//    sfs.sync().unwrap();
//}

#[test]
fn create_then_lookup() {
    let sfs = _create_new_sfs();
    let root = sfs.root_inode();

    assert!(Rc::ptr_eq(&root.borrow().lookup(".").unwrap(), &root), "failed to find .");
    assert!(Rc::ptr_eq(&root.borrow().lookup("..").unwrap(), &root), "failed to find ..");

    let file1 = root.borrow_mut().create("file1", FileType::File)
        .expect("failed to create file1");
    assert!(Rc::ptr_eq(&root.borrow().lookup("file1").unwrap(), &file1), "failed to find file1");
    assert!(root.borrow().lookup("file2").is_err(), "found non-existent file");

    let dir1 = root.borrow_mut().create("dir1", FileType::Dir)
        .expect("failed to create dir1");
    let file2 = dir1.borrow_mut().create("file2", FileType::File)
        .expect("failed to create /dir1/file2");
    assert!(Rc::ptr_eq(&root.borrow().lookup("dir1/file2").unwrap(), &file2), "failed to find dir1/file1");
    assert!(Rc::ptr_eq(&dir1.borrow().lookup("..").unwrap(), &root), "failed to find .. from dir1");

    sfs.sync().unwrap();
}

#[test]
fn rc_layout() {
    // [usize, usize, T]
    //  ^ start       ^ Rc::into_raw
    let p = Rc::new([2u8; 5]);
    let ptr = Rc::into_raw(p);
    let start = unsafe { (ptr as *const usize).offset(-2) };
    let ns = unsafe { &*(start as *const [usize; 2]) };
    assert_eq!(ns, &[1usize, 1]);
}

// #[test]
fn kernel_image_file_create() {
    let sfs = _open_sample_file();
    let root = sfs.root_inode();
    let files_count_before = root.borrow().list().unwrap().len();
    root.borrow_mut().create("hello2", FileType::File).unwrap();
    let files_count_after = root.borrow().list().unwrap().len();
    assert_eq!(files_count_before + 1, files_count_after);
    assert!(root.borrow().lookup("hello2").is_ok());

    sfs.sync().unwrap();
}

// #[test]
fn kernel_image_file_unlink() {
    let sfs = _open_sample_file();
    let root = sfs.root_inode();
    let files_count_before = root.borrow().list().unwrap().len();
    root.borrow_mut().unlink("hello").unwrap();
    let files_count_after = root.borrow().list().unwrap().len();
    assert_eq!(files_count_before, files_count_after + 1);
    assert!(root.borrow().lookup("hello").is_err());

    sfs.sync().unwrap();
}


// #[test]
fn kernel_image_file_rename() {
    let sfs = _open_sample_file();
    let root = sfs.root_inode();
    let files_count_before = root.borrow().list().unwrap().len();
    root.borrow_mut().rename("hello", "hello2").unwrap();
    let files_count_after = root.borrow().list().unwrap().len();
    assert_eq!(files_count_before, files_count_after);
    assert!(root.borrow().lookup("hello").is_err());
    assert!(root.borrow().lookup("hello2").is_ok());

    sfs.sync().unwrap();
}

// #[test]
fn kernel_image_file_move() {
    use core::ops::DerefMut;
    let sfs = _open_sample_file();
    let root = sfs.root_inode();
    let files_count_before = root.borrow().list().unwrap().len();
    root.borrow_mut().unlink("divzero").unwrap();
    let rust_dir = root.borrow_mut().create("rust", FileType::Dir).unwrap();
    root.borrow_mut().move_("hello", rust_dir.borrow_mut().deref_mut(), "hello_world").unwrap();
    let files_count_after = root.borrow().list().unwrap().len();
    assert_eq!(files_count_before, files_count_after + 1);
    assert!(root.borrow().lookup("hello").is_err());
    assert!(root.borrow().lookup("divzero").is_err());
    assert!(root.borrow().lookup("rust").is_ok());
    assert!(rust_dir.borrow().lookup("hello_world").is_ok());

    sfs.sync().unwrap();
}

#[test]
fn hard_link() {
    let sfs = _create_new_sfs();
    let root = sfs.root_inode();
    let file1 = root.borrow_mut().create("file1", FileType::File).unwrap();
    use core::ops::DerefMut;
    root.borrow_mut().link("file2", file1.borrow_mut().deref_mut()).unwrap();
    let file2 = root.borrow().lookup("file2").unwrap();
    file1.borrow_mut().resize(100);
    assert_eq!(file2.borrow().info().unwrap().size, 100);

    sfs.sync().unwrap();
}

#[test]
fn nlinks() {
    use core::ops::DerefMut;
    let sfs = _create_new_sfs();
    let root = sfs.root_inode();
    // -root
    assert_eq!(root.borrow().info().unwrap().nlinks, 2);

    let file1 = root.borrow_mut().create("file1", FileType::File).unwrap();
    // -root
    //   `-file1 <f1>
    assert_eq!(file1.borrow().info().unwrap().nlinks, 1);
    assert_eq!(root.borrow().info().unwrap().nlinks, 2);

    let dir1 = root.borrow_mut().create("dir1", FileType::Dir).unwrap();
    // -root
    //   +-dir1
    //   `-file1 <f1>
    assert_eq!(dir1.borrow().info().unwrap().nlinks, 2);
    assert_eq!(root.borrow().info().unwrap().nlinks, 3);

    root.borrow_mut().rename("dir1", "dir_1").unwrap();
    // -root
    //   +-dir_1
    //   `-file1 <f1>
    assert_eq!(dir1.borrow().info().unwrap().nlinks, 2);
    assert_eq!(root.borrow().info().unwrap().nlinks, 3);

    dir1.borrow_mut().link("file1_", file1.borrow_mut().deref_mut()).unwrap();
    // -root
    //   +-dir_1
    //   |  `-file1_ <f1>
    //   `-file1 <f1>
    assert_eq!(dir1.borrow().info().unwrap().nlinks, 2);
    assert_eq!(root.borrow().info().unwrap().nlinks, 3);
    assert_eq!(file1.borrow().info().unwrap().nlinks, 2);

    let dir2 = root.borrow_mut().create("dir2", FileType::Dir).unwrap();
    // -root
    //   +-dir_1
    //   |  `-file1_ <f1>
    //   +-dir2
    //   `-file1 <f1>
    assert_eq!(dir1.borrow().info().unwrap().nlinks, 2);
    assert_eq!(dir2.borrow().info().unwrap().nlinks, 2);
    assert_eq!(root.borrow().info().unwrap().nlinks, 4);
    assert_eq!(file1.borrow().info().unwrap().nlinks, 2);

    root.borrow_mut().rename("file1", "file_1").unwrap();
    // -root
    //   +-dir_1
    //   |  `-file1_ <f1>
    //   +-dir2
    //   `-file_1 <f1>
    assert_eq!(dir1.borrow().info().unwrap().nlinks, 2);
    assert_eq!(dir2.borrow().info().unwrap().nlinks, 2);
    assert_eq!(root.borrow().info().unwrap().nlinks, 4);
    assert_eq!(file1.borrow().info().unwrap().nlinks, 2);

    root.borrow_mut().move_("file_1", dir2.borrow_mut().deref_mut(), "file__1").unwrap();
    // -root
    //   +-dir_1
    //   |  `-file1_ <f1>
    //   `-dir2
    //      `-file__1 <f1>
    assert_eq!(dir1.borrow().info().unwrap().nlinks, 2);
    assert_eq!(dir2.borrow().info().unwrap().nlinks, 2);
    assert_eq!(root.borrow().info().unwrap().nlinks, 4);
    assert_eq!(file1.borrow().info().unwrap().nlinks, 2);

    root.borrow_mut().move_("dir_1", dir2.borrow_mut().deref_mut(), "dir__1").unwrap();
    // -root
    //   `-dir2
    //      +-dir__1
    //      |  `-file1_ <f1>
    //      `-file__1 <f1>
    assert_eq!(dir1.borrow().info().unwrap().nlinks, 2);
    assert_eq!(dir2.borrow().info().unwrap().nlinks, 3);
    assert_eq!(root.borrow().info().unwrap().nlinks, 3);
    assert_eq!(file1.borrow().info().unwrap().nlinks, 2);

    dir2.borrow_mut().unlink("file__1").unwrap();
    // -root
    //   `-dir2
    //      `-dir__1
    //         `-file1_ <f1>
    assert_eq!(file1.borrow().info().unwrap().nlinks, 1);
    assert_eq!(dir1.borrow().info().unwrap().nlinks, 2);
    assert_eq!(dir2.borrow().info().unwrap().nlinks, 3);
    assert_eq!(root.borrow().info().unwrap().nlinks, 3);

    dir1.borrow_mut().unlink("file1_").unwrap();
    // -root
    //   `-dir2
    //      `-dir__1
    assert_eq!(file1.borrow().info().unwrap().nlinks, 0);
    assert_eq!(dir1.borrow().info().unwrap().nlinks, 2);
    assert_eq!(dir2.borrow().info().unwrap().nlinks, 3);
    assert_eq!(root.borrow().info().unwrap().nlinks, 3);

    dir2.borrow_mut().unlink("dir__1").unwrap();
    // -root
    //   `-dir2
    assert_eq!(file1.borrow().info().unwrap().nlinks, 0);
    assert_eq!(dir1.borrow().info().unwrap().nlinks, 0);
    assert_eq!(root.borrow().info().unwrap().nlinks, 3);
    assert_eq!(dir2.borrow().info().unwrap().nlinks, 2);

    root.borrow_mut().unlink("dir2").unwrap();
    // -root
    assert_eq!(file1.borrow().info().unwrap().nlinks, 0);
    assert_eq!(dir1.borrow().info().unwrap().nlinks, 0);
    assert_eq!(root.borrow().info().unwrap().nlinks, 2);
    assert_eq!(dir2.borrow().info().unwrap().nlinks, 0);

    sfs.sync().unwrap();
}