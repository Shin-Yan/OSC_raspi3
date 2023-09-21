# OSC2023

| Github Account | Student ID | Name          |
|----------------|------------|---------------|
| Shin-Yan       | 310555008  | 曾信彥        |

## Requirements

* a cross-compiler for aarch64
* (optional) qemu-system-arm

## Build 

```
make
```

## Test With QEMU
place your dtb file in the img/ directory
```command
$ cp bcm2710-rpi-3-b-plus.dtb <osc_dir>/img/.
$ make qemuk
```

## Lab0/1 Supported
* mini uart I/O 
* Simple shell 
* Mailbox communication 
* reboot

## Lab2 Supported
* UART Bootloader and self relocation
* Devicetree parsing and get address of initramfs

## Lab3 supported
* Interrupt Vector Table
* Exception handling
* Rpi3 Peripheral Interrupt
* Timer Multiplexing
* Concurrent I/O Devices Handling

## Lab4 supported
* Buddy system
* Dynamic Memory Allocator
* Efficient Page Allocation
* reserved memory
* Startup Allocation

## Lab5 supported
* Thread
* User process and system call
* user program: Video Player
* POSIX signal

## Lab6 supported
* Virtual Memory in Kernel space
* Virtual Memory in user space
* mmap
* Page Fault Handler & Demand Paging

## Lab7 supported
* Virtual filesystem
* /dev/uart
* /dev/framebuffer

## Lab8
* FAT filesystem
* Open and Read
* Create and write
* Memory cached SD card