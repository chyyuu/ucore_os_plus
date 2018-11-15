# uCore OS by Rust Lang, supporting riscv32

## Summary

This is a training project of THU Operating System (2018 Autumn).

## Building

### Environment

* Rust toolchain: rustc 1.32.0-nightly (36a50c29f 2018-11-09)
*  `cargo-xbuild`
* `QEMU` >= 2.12.0
* riscv32
  * RISCV32 GNU toolchain

```
### setup environment
sudo apt install libsdl2-dev
cd DIR
git clone https://github.com/riscv/riscv-qemu.git
cd riscv-qemu
./configure --target-list=riscv32-softmmu
make -j8
rustup upgrade
cargo install cargo-xbuild
rustup override set nightly-2018-11-09
// setenv for qemu-system-riscv32 and riscv32-unknown-e3lf-gcc
export PATH=PATH of qemu-system-riscv32:PATH of riscv32-unknown-e3lf-gcc:$PATH

### How to run

git clone https://github.com/chyyuu/RustOS.git
cd RustOS
//LABX is one of lab1-rv32-showstr lab1-rv32-interrupt lab2-rv32-paging
        lab3-rv32-swaping lab4-rv32-thread lab5-rv32-usrprocess
        lab6-rv32-scheduling lab7-rv32-mutexsync lab8-rv32-fs
git checkout LABX 
cd RustOS/kernel
make run arch=riscv32
```

## License

The source code is licensed under the Apache License (Version 2.0).

## Contact
Runji Wang <wangrunji0408@163.com>
Zhengyang Dai <daizy15@mails.tsinghua.edu.cn>
Yu Chen <yuchen@tsinghua.edu.cn>
Yong Xiang <xyong@tsinghua.edu.cn>
