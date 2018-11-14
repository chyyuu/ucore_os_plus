# SimpleFileSystem Rust移植报告

王润基 2018.05.05

## 任务目标

* 用Rust重新实现SFS模块，力求精简
* 以crate库的形式发布，不依赖OS，支持单元测试
* 导出兼容ucore的C接口，能链接到ucore lab8中替换原有实现
* 在RustOS中使用

## 结构设计

自底向上：

#### 硬盘数据结构层

和SFS在硬盘上具体的存储结构相关。位于`structs.rs`。

这部分定义了硬盘上的数据结构，完全照搬C中的定义，并实现了一些Rust辅助方法。

#### SFS层

和SFS在内存中的对象相关。位于`sfs.rs`。

这部分主要定义了两个对象：`SimpleFileSystem`和`INode`。

它们依赖下层的具体结构，实现上层VFS的接口，完成具体的文件操作。

由于利用了Rust的一些模块的特性，这两个结构体的定义和C中很不一样。

#### VFS层

文件系统通用接口。位于`vfs.rs`。

这部分主要定义了三个接口：`FileSystem`，`INode`，`Device`。

其中前两个是需要具体文件系统实现的。Device是依赖项，提供读写方法。

它们基本照搬C中的定义，但换成了Rust风格，更加本质。

由于C在语言层面缺乏对接口的支持，ucore中是用struct和函数指针实现接口功能。但在Rust中就是简单的Trait。

#### C兼容层

将Rust风格的VFS导出为ucore可用的C接口。位于`c_interface.rs`。

这部分导入了ucore中`stat` `iobuf` `device`等结构，将他们实现Rust的Trait。同时将VFS层中的Trait转化成C函数接口。

除此之外，还要导入ucore中的一些基础设施，例如`kmalloc` `kfree` `cprintf` `panic`

## Rust带来的好处

* 利用RAII特性，使用一些小Wrapper结构，进行自动标记和断言，大大减轻开发者心智负担。例如：Mutex锁，RefCell访问检查，Dirty脏标记，Rc引用计数。
* 从始至终严格的访问控制，只要不滥用unsafe，可快速并行化，并保证安全。
* 语言描述能力强，代码量少。对比C的SFS模块1100+行，Rust只用了700+行（除去单元测试和C兼容层）。

## 接下来的目标

* 接入ucore中能够跑起来
* 实现mksfs等周边工具
* 多线程支持及并行优化



## 与ucore_os_lab8 对接报告

王润基 2018.05.07

搞定这事儿耗费了大约15小时的时间

### 实现的功能

通过C兼容层将Rust VFS接口和ucore VFS层对接起来。

经测试，在shell中能够正常执行各个程序，`ls`输出正确。

在实现上：清理掉ucore_os_lab8的sfs文件夹，只留下`sfs.h`（未修改）和`sfs.c`（添加少许辅助函数）。再将Rust SFS编译成静态库，链接到kernel中。

### 如何运行

整合之后的ucore可以从[这里](https://github.com/wangrunji0408/ucore_os_lab/tree/rust-fs/labcodes_answer/lab8_result)获得。

```bash
# 下载项目
git clone https://github.com/wangrunji0408/ucore_os_lab.git -b rust-fs --recursive 
# 切到lab8目录
cd ucore_os_lab/labcodes_answer/lab8_result/
# 如果没有安装Rust，用以下命令，但愿能work
make -f rust_sfs/Makefile install-rust
# 编译运行
make qemu
```

### 如何调试

我使用CLion + Rust插件编写代码。

调试时，首先用`make dbg4ec`开启QEMU，再用CLion中自带的`GDB Remote Debug`连接上去，即可GUI调试。此外还需在ucore的makefile中添加编译参数`-g`，才能在CLion中定位C的源代码。

### 遇到的问题

- 链接丢失：

  ucore_os_lab的linker script少写了一些section，包括`*.data.*` `*.got.*` `*.bss.*`，导致Rust lib链接过去后丢失了一些段，比较坑的是这不会有任何提示。没有成功重定位的地址都是0，运行时直接Page fault。为了找出出错位置，各种gdb，objdump全上了，还得看汇编追踪寄存器，真是大坑。

- 引用LLVM内置函数：

  Rust lib会引用一些LLVM内置函数（如udivdi3，都是除法运算相关），链接时会报undefined symbol。但实际上并没有代码用到它们。《Writing an OS in Rust》中提到了这个问题，它的解决方案是链接时加`--gc-sections`选项，将未用到的段删掉，结果我对ucore如此操作之后所有段都没了，都boot不起来。。。最后是在C中强行定义这些符号解决的。

- 对ucore VFS接口理解的歧义：

  例如：`getdirentry`，`gettype`

  经常需要查看ucore SFS的原始实现，以及输出它传给我的结构内容，才能理解它到底是如何工作的。

### C兼容层的设计

#### ucore VFS分析

```c
// [1] SFS模块的入口点。从这里获得fs。
int sfs_do_mount(struct device *dev, struct fs **fs_store);

struct fs {
    // 头部是具体文件系统的内容
    union {
        struct sfs_fs __sfs_info;                   
    } fs_info;
    
    // 元信息
    enum {
        fs_type_sfs_info,
    } fs_type;
    
    // 虚函数表
    int (*fs_sync)(struct fs *fs);
      // [2] 从这里获得inode
    struct inode *(*fs_get_root)(struct fs *fs);
    int (*fs_unmount)(struct fs *fs);
    void (*fs_cleanup)(struct fs *fs);
};

struct inode {
    // 头部是具体文件系统的内容
    union {
        struct device __device_info;
        struct sfs_inode __sfs_inode_info;
    } in_info;
    
    // 元信息，引用计数
    enum {
        inode_type_device_info = 0x1234,
        inode_type_sfs_inode_info,
    } in_type;
    int ref_count;
    int open_count;
    
    // 引用fs
    struct fs *in_fs;
    
    // 虚函数表
      // [3] 通过其中的lookup()/create()获得其他inode
    const struct inode_ops *in_ops;
};
```

#### Rust VFS分析

```rust
type INodePtr = Rc<RefCell<INode>>;

pub trait FileSystem {
    // [1] 从这里获得FS
    fn new(device: Box<Device>) -> Result<FileSystem>;
    // [2] 从这里获得INode
    fn root_inode(&self) -> INodePtr;
    // 其他函数...
}

pub trait INode {
    // 引用fs
    fn fs(&self) -> Weak<FileSystem>;
    // [3] 获得其他INode
    fn lookup(&self, path: &'static str) -> Result<INodePtr>;
    // 其他函数...
}
```

#### 如何合并

ucore VFS中fs和inode头部是具体FS的struct。Rust VFS使用Rc指针相互引用。为了将它们合并起来，考虑过两种方案：

* **在头部放Rc指针**
  * Rust VFS = ucore SFS
  * 还需建立Rust INode => ucore INode的反向引用
    * 要么侵入式地增加Rust INode的字段
    * 要么**在兼容层搞一个全局Map**
  * 这种方式耦合较低，但多一层指针跳转，性能可能略差。
* 在头部放Rust SFS的结构本体
  * Rust VFS = ucore VFS
  * 需要在Rust new出SFS结构时做文章， 委托ucore分配多一点空间并做VFS的初始化，得魔改Rust的全局内存分配器。
  * 这种方式耦合较高，需对Rust结构的实际内存布局有深入理解。

最终采取的方案加粗表示。

在这个框架下，将ucore虚函数表中的每一个都用RustVFS提供的接口去实现，并处理参数格式转化。以比较复杂的`lookup`为例：

```rust
// 这个函数的地址保存在虚表中，传给ucore
extern fn lookup(inode: &mut INode, path: *mut u8, inode_store: &mut *mut INode) -> ErrorCode {
    // 【参数处理】将C字符串转为Rust字符串
    let path = unsafe{ libc::from_cstr(path) };
    // 【完成功能】调用 Rust VFS 的函数
    let target = inode.borrow().lookup(path);
    // 【结果处理】
    match target {
        Ok(target) => {
            // 从ucore inode中获取ucore fs
            let fs = unsafe{ ucore::inode_get_fs(inode) };
            // 使用全局Map，将Rust INode映射为ucore inode，如果不存在则新建一个(要用到ucore fs)
            let inode = INode::get_or_create(target, fs);
            // 按ucore接口要求，增加引用计数
            unsafe { ucore::inode_ref_inc(inode) };
            // 给出ucore inode
            *inode_store = inode;
            ErrorCode::Ok
        },
        // 设置正确的错误码
        Err(_) => ErrorCode::NoEntry,
    }
}
```

经过统计，`C兼容层`的有效代码量约500行，和实现核心功能的`Rust SFS层`相当。说明这种胶水代码真是又臭又长，搞兼容性都是脏活累活。

### 那么……意义何在？

* 将FS模块接入实际OS，验证其可行性和可靠性（尤其是在RustOS还遥遥无期的背景下……）
* 检验了Rust和C的互操作性，证明Rust编写系统模块是可行的。
* （试图）为ucore提供一套更好更实用的文件系统接口（个人认为Rust接口更容易去实现），方便日后将其他FS接入（如CS140e.FAT32）
* （也许）能为OS教学提供另一种可能性
* 锻炼了我的跨语言+汇编级别debug能力，让我对ucore FS有更深入的了解