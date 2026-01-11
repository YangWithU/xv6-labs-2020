
# refs

1. 
- https://blog.miigon.net/posts/s081-ending/
参考博客

2.
- https://pdos.csail.mit.edu/6.S081/2020/labs/guidance.html
官网

3.
- https://mit-public-courses-cn-translatio.gitbook.io/mit6-s081/lec01-introduction-and-examples/1.1-ke-cheng-jian-jie
课程内容的翻译

4.
- https://th0ar.gitbooks.io/xv6-chinese/content/content/chapter1.html
xv6中文文档

5. 
- https://csdiy.wiki/%E6%93%8D%E4%BD%9C%E7%B3%BB%E7%BB%9F/MIT6.S081/
csdiy页面

6. https://jyywiki.cn/OS/2022/index.html
jyy 也有一些辅助资料

7. https://github.com/Aneureka/xv6-riscv
很好的参考资料

8. https://twdlll.github.io/2020/11/10/6.S081-2/
优先看这个

9. https://www.cnblogs.com/KatyuMarisaBlog/p/13727565.html
如何调试用户态程序

10. https://blog.miigon.net/posts/s081-lab1-unix-utilities/
参考答案

11. https://blog.wingszeng.top/mit-6-s081-lab-2/
参考答案

用 gdb-multiarch 调试

# 计划和进度

2025-12-26: 打算开始搞2020年版本的lab
不想再看一堆的教材


# qemu 无法启动

make qemu
之后卡死，持续占用3个cpu核心

最后让ai修改
xv6-labs-2020/kernel/riscv.h
给最后加上

```c

// PMP (Physical Memory Protection)
#define PMP_R (1L << 0)
#define PMP_W (1L << 1)
#define PMP_X (1L << 2)
#define PMP_A_TOR (1L << 3)
#define PMP_A_NA4 (2L << 3)
#define PMP_A_NAPOT (3L << 3)
#define PMP_L (1L << 7)

static inline void
w_pmpcfg0(uint64 x)
{
  asm volatile("csrw pmpcfg0, %0" : : "r" (x));
}

static inline void
w_pmpaddr0(uint64 x)
{
  asm volatile("csrw pmpaddr0, %0" : : "r" (x));
}
```

xv6-labs-2020/kernel/start.c
```c
void
start()
{
  // set M Previous Privilege mode to Supervisor, for mret.
  unsigned long x = r_mstatus();
  x &= ~MSTATUS_MPP_MASK;
  x |= MSTATUS_MPP_S;
  w_mstatus(x);

  // set M Exception Program Counter to main, for mret.
  // requires gcc -mcmodel=medany
  w_mepc((uint64)main);

  // disable paging for now.
  w_satp(0);

  // delegate all interrupts and exceptions to supervisor mode.
  w_medeleg(0xffff);
  w_mideleg(0xffff);
  w_sie(r_sie() | SIE_SEIE | SIE_STIE | SIE_SSIE);

  // ask for clock interrupts.
  timerinit();

  // keep each CPU's hartid in its tp register, for cpuid().
  int id = r_mhartid();
  w_tp(id);

  // configure PMP to let supervisor mode access all of physical memory.
  w_pmpaddr0(0x3fffffffffffffull);
  w_pmpcfg0(PMP_R | PMP_W | PMP_X | PMP_A_NAPOT);

  // switch to supervisor mode and jump to main().
  asm volatile("mret");
}
```


加上这一段好了
原因：QEMU 版本过新（8.2.2），默认启用了严格的 PMP（物理内存保护）检查。未配置 PMP 时，S 模式无法访问内存。
在内核启动代码（start.c）中添加 PMP 配置，允许 S 模式访问所有物理内存。

## 总结 之后可以发给ai

xv6-labs-2020/kernel/start.c
xv6-labs-2020/kernel/riscv.h

总结一下我们解决这个问题的过程：

问题现象：make qemu 后卡死，无任何输出。
初步诊断：怀疑是编译器版本过新导致的 PIE（位置无关代码）问题。
第一次尝试：注释掉 Makefile 中的 PIE 检测代码。
结果：失败。
原因：你的编译器默认开启 PIE，注释掉检测代码反而导致没有加上 -fno-pie，生成了错误的内核代码（包含 GOT 表访问）。
第二次尝试：强制在 CFLAGS 中添加 -fno-pie -no-pie。
结果：失败。
原因：make 规则中编译汇编文件（.S）时没有使用 CFLAGS，导致 entry.S 依然被编译成了 PIE 格式。
第三次尝试：修改 Makefile 规则，强制汇编文件编译时使用 CFLAGS。
结果：失败（虽然代码格式对了，但依然卡死）。
原因：QEMU 版本过新（8.2.2），默认启用了严格的 PMP（物理内存保护）检查。未配置 PMP 时，S 模式无法访问内存。
最终解决：在内核启动代码（start.c）中添加 PMP 配置，允许 S 模式访问所有物理内存。
结果：成功！


# 退出

Ctrl-a x

# usys.S 是干啥的

tldr:

```asm
.global sleep
sleep:
 li a7, SYS_sleep
 ecall
 ret
```

//LINK - 编译过程简单讲解
1. `.global sleep` 告诉汇编器 sleep 是个全局的符号

(1) 编译sleep.c时候，找不到sleep()定义，则生成一个“未解析的符号引用”(Unresolved Symbol Reference)标明让后面过程再填上地址
(2) 汇编阶段，输入：usys.S (由 Perl 脚本生成) 汇编器将汇编代码翻译成二进制机器码，生成 usys.o;
usys.S中包含.global全局符号 sleep, 那么汇编器就会在usys.o的符号表（Symbol Table）生成全局的符号
(3) 链接阶段，输入：sleep.o + usys.o + 其他库文件，链接器（ld）负责把这些零散的 .o 文件拼成一个完整的可执行文件
a. 链接器拿起 sleep.o，看到那个便条：“我需要 sleep 的地址”
b. 链接器四处寻找，最后在 usys.o 的符号表里找到了 sleep（因为它被标记为 .global 了）
c. 填空：链接器把 usys.o 里 sleep 函数的真实地址，填回到 sleep.o 那个留白的地方


2. `li a7, SYS_sleep` i 是 "Load Immediate"（加载立即数）, SYS_sleep 是一个宏（定义在 kernel/syscall.h 中），比如是数字 13。
含义：把数字 13 放到寄存器 a7 中。
为什么是 a7？ 这是 RISC-V 的约定。内核约定好了，每次有人敲门（ecall），它就先看 a7 里的数字，以此判断具体调用哪个函数

编译器（gcc）将其翻译成汇编代码时，会严格遵守这个约定：

(1) 参数传递：前 8 个整数型参数依次放入寄存器 a0 到 a7。
(2) 返回值：函数返回值放在 a0。

3. `ecall` Environment Call 的缩写。用户态通往内核态的唯一大门。执行这条指令后，CPU 会暂停当前程序，提升权限等级，跳转到内核预设的入口地址（trampoline）



# 调用系统调用sleep之后发生的事情

1. 用户态程序调用系统调用sleep
2. 转到 user/usys.S 中的 sleep 符号 (因为之前编译的时候就是按照这个汇编中的sleep符号进行编译的，所以会跳到这里)
3. 把 SYS_sleep 的编号13加载到 a7 寄存器
4. ecall, 发生系统中断, 中断处理函数跑到如下堆栈：

#0  sleep (chan=chan@entry=0x8000b020 <ticks>, lk=lk@entry=0x80019768 <tickslock>) at kernel/proc.c:559
#1  0x0000000080002d56 in sys_sleep () at kernel/sysproc.c:72
#2  0x0000000080002bdc in syscall () at kernel/syscall.c:140
#3  0x00000000800028c6 in usertrap () at kernel/trap.c:67
#4  0x000000000000003c in ?? ()


5. usertrapret() 返回用户态


