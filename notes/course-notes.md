# Lecture01
## Why learning X?
目的：
- 理解其基本动机、基本方法、里程碑、走过的弯路
- 最终目的：应用、创新、革命

操作系统？
- 动机：更快更好的服务更多应用
- 方法：Building Abstractions
- 里程碑：UNIX, Linux, ...

## 操作系统发展历史
- 硬件
- 软件
- 操作系统

1940s:
- 电子真空管
## 课后
OSTP: Operating Systems: Three Easy Pieces:
- Dialogue on the Book
- Introduction to Operating Systems

GPT-4: better than GPT-3.5
# Lecture02 应用视角的操作系统
操作系统作为软硬件的桥梁，首先对其服务对象，即应用程序进行了解。
程序是什么？状态机
- 状态
- 初始状态
- 状态迁移

## 编译器与编译优化
|: 将命令的输出传递给另一个命令
&: 将stderr重定向至与stdout相同的位置

编译器
- input：高级语言 状态机

- output：汇编代码 状态机
> 编译器是状态机之间的翻译器

C编译器优化
- 函数内联，将函数调用替换为函数体本身内容
- 常量传播，计算常量表达式的值并进行替换
- 死代码消除，删除永远不会被执行到的代码

系统调用是使得程序计算结果可见的唯一方法
- 不改变语义 = 不改变可见结果
- 状态机视角：syscall序列完全一致，任何优化都可以

C中的不可优化部分
- 外部函数调用
- `volatile[load|store|inline assembly]`

在线汇编分析
- https://godbolt.org/z/f698djMTq

## 课后
web sites:
https://www.gnu.org/software/coreutils/
- GNU Coreutils
- GNU Binutils

gdb
https://sourceware.org/gdb/current/onlinedocs/gdb.html/
- Reverse Execution
- TUI
- ...

### GDB Text User Interface TUI
> The GDB Text User Interface (TUI) is a terminal interface which uses the curses library to show the source file, the assembly output, the program registers and GDB commands in separate text windows

TUI
- layout src
- layout asm
- ...







toybox busybox: commands source code

Tasks:
- strace mini program
- <font color="red">扩展非递归汉诺塔问题至f与g两个函数互相调用</font>
- strace更多的命令，如ls pwd，了解部分系统的作用

```bash
$ strace -o trace.log ls ./|awk -F'(' '{print $1}' trace.log | sort | uniq
+++ exited with 0 +++
access
arch_prctl
brk
close
execve
exit_group
futex
getdents64
getrandom
ioctl
mmap
mprotect
munmap
newfstatat
openat
pread64
prlimit64
read
rseq
set_robust_list
set_tid_address
statfs
statx
write
```

### 汉诺塔问题

递归vs非递归




程序自己无法停下来
- 只能借助操作系统
利用syscall借助操作系统操作资源


应用程序 = 计算 + 操作系统API
- 窗口管理器
  - 管理屏幕设备
  - 与其他进程通信
- 任务管理器
  - 访问操作系统提供的进程对象，M1-pstree
- 杀毒软件
  - 静态（read），动态（ptrace）

### 状态机
- 严格数学定义
  - 状态
    - [StackFrame, StackFrame, ...] + 全局变量

  - 初始状态
    - 仅有一个StarckFrame(main, argc, argv, PC=0)
    - 全局变量为初始值

  - 状态迁移
    - 执行frames[-1]，即top StackFrame.PC处的语句

# Lecture03 硬件视角的操作系统
- 什么是计算机（硬件）系统
- 约定

状态机模型，不同层次
- C代码，高级语言
  - SimpleC & GDB(TUI src)
- 汇编
  - GDB(TUI asm)
- 处理器
  - mini-rv32ima; ICS PA
- 数字电路
  - Logisim


## 计算机系统的状态机模型

状态
- 内存、寄存器的数值
- 外部世界
  - 设备上的寄存器、I/O、memory-mapped
  - Interrupt/Reset Line
  - 客观存在，但计算机系统不能直接访问

初始状态
- 由系统设计者规定 CPU Reset

状态迁移
- 从PC取指令执行
- 响应中断 if(intr) goto vec;
- 输入输出


Reset电路：热启动
操作系统如何感知外部世界

CPU Reset: 其他体系结构

RISC-V:
- PC无规定
- 寄存器除了x0，全部undefined
- CSR 控制状态寄存器
  - 省电路
  - 


软件控制一切

CPU RESET >> Firmware厂商将代码规定在ROM中

## Firmware

加载Master Boot Record MBR
第一个可启动磁盘前512字节进行加载

现在有了UEFI标准，可以加载任意GPT分区表上的FAT分区中存储的应用


Firmware BIOS系统
- 计算机系统配置：CPU电压、接口开关
- 加载操作系统
https://www.qemu.org/docs/master/system/invocation.html#hxtool-8

DOS时代，BIOS中断，直接调用BIOS代码

CPU Reset后初始化硬件；对接操作系统Boot Loader

Leagcy BIOS >> UEFI

Leagcy BIOS
- IBM PC
- 兼容机百花齐放，AMI Phoenix BIOS


UEFI Unified Extensible Firmware Interface



1998:

Firmware通常是只读的
但是 Firmware也需要更新

Intel 430TX芯片组允许写入PROM
- 某些主板有写保护跳线

CIH病毒
- 可以破坏硬件
- cih-1.4.asm
https://github.com/onx/CIH/blob/master/cih-1.4.asm


1983:
Firmware BIOS加载前512字节到0x7c00
- 如果最后是 0x55 0xAA
- 01010101 10101010

加载过程 

我们可以写446=512 -2(55 aa) -64(分区表)

Grub
- 扫描磁盘，找到ELF文件头
- 该ELF文件是Grub，弹窗
- 加载Linux Kernel