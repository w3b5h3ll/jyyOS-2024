## A Dialogue on the Book
All about
- virtualization
- concurrency
- persistence

> As Confucius said...

> It seemed to follow. Also, I am a big fan of Confucius, and even bigger fan of Xunzi, who actually is a better source for this quote

> Well, I think it is sometimes useful to pull yourself outside of a narrative  and think a bit;



## Introduction to Operating System

some reading books:
- PP03
- BOH10

a program runs(<font color="red">Von Neumann modes</font>):
- cpu fetches an instruction from memory
- decodes
- executes
- moves on to next instruction

Operating System: easy(<font color="red">correctly and efficiently</font>) to use, such as follow:
- allowing programs to share memory
- run many at the same time
- ...


如何实现呢？虚拟化：将物理世界与软件世界连系起来
> Thus, we sometimes refer to the operating sytem as a virtual machine

> OS is sometimes known as a resource manager

OS provides some interfaces(APIs)
- system call

> "Control-c" we can halt the program

> It turns out that the operating system, with some help from the hardware, is in charge of the illusion.
> - virtualizing the CPU

### Virtualizing Memory

### Concurrency

> Unfortunately, the problems of concurrency are no longer limited just to the OS itself. Indeed, such as multi-threaded programs

> You can think of a thread as a function running within the same memory space as other functions

how instructions are executed?(three instructions)
这三条指令不会同时执行，即不是原子性的
- one to load the value of the counter from memory into a register
- one to increment it
- one to store it back into memory

并发导致了一致性问题

### Persistence 持久化

DRAM store values in a volatile manner, data maybe lost

- SSD
- file system, manages the disk

> it is assumed that often times, users will want to share information that is in files.

> You might be wondering what the OS does in order to actually write to disk.

The file system has to do a fair bit of work:
1. find where
2. track it
3. 

写device driver复杂性

为了性能考虑，多种技术提出
- delay such writes for a while
- journaling or copy-on-write

不同的file system数据结构
- simple lists
- b-trees
- ...

Design Goals
- finding the right set of trade-offs is a key to building systems
- 抽象，加层
- perfoermance
- minimize the overheads of the OS
- protection: isolation
- reliability
- Others goals
  - energy-efficiency
  - security
  - mobility
- ...





> we'll discuss some of the major abstractions that have developed over time, giving you a way to think about pieces of the OS




### Some History

Early OS: just libraries
- 大型机
  - 非交互，只能批处理，价格昂贵


Beyond Libraries: protection
- the idea of system call was invented
  - 添加硬件指令，使得OS受控
  - 系统调用将控制权转移到OS中
  - user mode >> kernel mode, 中断机制trap


The Era of Multiprogramming
> Not surprisingly, one of the major impacts of this drop in cost was an increase in developer activity; more smart people got their hands on computers and thus made computer systems do more interesting and beautiful things.

The Modern Era
- personal computer PC

### Summary
...





