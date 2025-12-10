# Build instructions for Terrakernel

To build Terrakernel you have to firstly make sure you have those programs:
```
qemu-system-x86
build-essential
binutils
tar
```

To install them use:
```bash
sudo apt install qemu-system-x86 build-essential binutils tar mtools
```

Before running the kernel make sure to prepare initrd.img:
```bash
make initrd
```

To run the OS use the following command:
```bash
make run
```

By default the OS runs on x86_64 UEFI hardware
If you want to run the OS on x86_64 BIOS you will use make run-bios

For some reason `make run-hdd` doesn't work, so just use `make run` with ISO :meme: