# M1 pstree
https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap12.html

pstree [OPTION]...

> Unix is user-friendly -- it's just choosy about who its friends are

UNIX
- Keep it simple, stupid
- Everything is a file


Everything is a file VS type-safe API
- UNIX VS Windows Kernel + GDI


## 问题分解
1. get args from command line
2. get pid from /proc/...
3. for pid in list[], get ppid
4. 建立树，根据命令行参数决定是否排序
5. 打印



32bit支持
```bash
sudo apt-get install  build-essential libc6-dev libc6-dev-i386
```

procfs
- https://en.wikipedia.org/wiki/Procfs

```bash

       /proc/[pid]/status
              Provides much of the information in /proc/[pid]/stat and /proc/[pid]/statm in a format that's easier for humans to parse.  Here's an example:

                  $ cat /proc/$$/status
                  Name:   bash
                  Umask:  0022
                  State:  S (sleeping)
                  Tgid:   17248
                  Ngid:   0
                  Pid:    17248
                  PPid:   17200
                  TracerPid:      0
                  Uid:    1000    1000    1000    1000
                  Gid:    100     100     100     100
                  FDSize: 256
                  Groups: 16 33 100
                  NStgid: 17248
                  NSpid:  17248
                  NSpgid: 17248
                  NSsid:  17200
```



## 思考题
使用Windows的任务管理器“画图”
