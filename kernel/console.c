//
// Console input and output, to the uart.
// Reads are line at a time.
// Implements special input characters:
//   newline -- end of line
//   control-h -- backspace
//   control-u -- kill line
//   control-d -- end of file
//   control-p -- print process list
//

#include <stdarg.h>

#include "defs.h"
#include "file.h"
#include "fs.h"
#include "memlayout.h"
#include "param.h"
#include "proc.h"
#include "riscv.h"
#include "sleeplock.h"
#include "spinlock.h"
#include "types.h"

#define BACKSPACE 0x100
#define C(x) ((x) - '@') // Control-x
#define MAX_HISTORY 128

//
// send one character to the uart.
// called by printf, and to echo input characters,
// but not from write().
//
void consputc(int c) {
  if (c == BACKSPACE) {
    // if the user typed backspace, overwrite with a space.
    uartputc_sync('\b');
    uartputc_sync(' ');
    uartputc_sync('\b');
  } else {
    uartputc_sync(c);
  }
}

struct {
#define INPUT_BUF 128

  struct spinlock lock;

  // input
  uint r; // Read index
  uint w; // Write index
  uint e; // Edit index
  char buf[INPUT_BUF];

  uint hist_msg_id; // 当前历史记录到哪一行
  uint view_idx;    // 当前正在查看哪一行
  char history[MAX_HISTORY][INPUT_BUF];
} cons;

//
// user write()s to the console go here.
//
int consolewrite(int user_src, uint64 src, int n) {
  int i;

  acquire(&cons.lock);
  for (i = 0; i < n; i++) {
    char c;
    if (either_copyin(&c, user_src, src + i, 1) == -1)
      break;
    uartputc(c);
  }
  release(&cons.lock);

  return i;
}

//
// user read()s from the console go here.
// copy (up to) a whole input line to dst.
// user_dist indicates whether dst is a user
// or kernel address.
//
// LINK - 用户态read调用后内核的最底层处理函数
int consoleread(int user_dst, uint64 dst, int n) {
  uint target;
  int c;
  char cbuf;

  target = n;
  acquire(&cons.lock);
  while (n > 0) {
    // wait until interrupt handler has put some
    // input into cons.buffer.
    while (cons.r == cons.w) {
      if (myproc()->killed) {
        release(&cons.lock);
        return -1;
      }
      sleep(&cons.r, &cons.lock);
    }

    c = cons.buf[cons.r++ % INPUT_BUF];

    if (c == C('D')) { // end-of-file
      if (n < target) {
        // Save ^D for next time, to make sure
        // caller gets a 0-byte result.
        cons.r--;
      }
      break;
    }

    // copy the input byte to the user-space buffer.
    // NOTE - 内核将读取到的数据送回用户态
    cbuf = c;
    if (either_copyout(user_dst, dst, &cbuf, 1) == -1)
      break;

    dst++;
    --n;

    if (c == '\n') {
      // a whole line has arrived, return to
      // the user-level read().
      break;
    }
  }
  release(&cons.lock);

  return target - n;
}

void save_to_cons_history(uint len) {
  uint pos = 0;
  for (pos = 0; pos < len; pos++) {
    char c = cons.buf[(cons.w + pos) % INPUT_BUF];
    if (c == '\n') {
      break;
    }
    cons.history[cons.hist_msg_id % MAX_HISTORY][pos] = c;
  }
  cons.history[cons.hist_msg_id % MAX_HISTORY][pos] = '\0';

  // 如果命令为空，则不保存
  if (pos == 0)
    return;

  cons.hist_msg_id++;
  cons.view_idx = cons.hist_msg_id;
}

//
// the console input interrupt handler.
// uartintr() calls this for input character.
// do erase/kill processing, append to cons.buf,
// wake up consoleread() if a whole line has arrived.
//
void consoleintr(int c) {
  acquire(&cons.lock);

  if (c == 2) { // 替换屏幕上的内容为last history
    // printf("got ctrl+b\n");
    if (cons.view_idx > 0) {
      cons.view_idx--;
    } else {
      // 已经是第一条了，无法再回退，直接释放锁并返回
      release(&cons.lock);
      return;
    }

    // 1. 擦除
    while (cons.e != cons.w && cons.buf[(cons.e - 1) % INPUT_BUF] != '\n') {
      cons.e--;
      consputc(BACKSPACE);
    }

    // 2. 打印并写入buf
    // 使用临时变量 len 获取长度，避免多次访问复杂数组结构
    int len = strlen(cons.history[cons.view_idx % MAX_HISTORY]);
    for (uint i = 0; i < len; i++) {
      char cc = cons.history[cons.view_idx % MAX_HISTORY][i];
      consputc(cc);
      cons.buf[cons.e++ % INPUT_BUF] = cc;
    }

    release(&cons.lock);
    return;
  }

  switch (c) {
  case C('P'): // Print process list.
    procdump();
    break;
  case C('U'): // Kill line.
    while (cons.e != cons.w && cons.buf[(cons.e - 1) % INPUT_BUF] != '\n') {
      cons.e--;
      consputc(BACKSPACE);
    }
    break;
  case C('H'): // Backspace
  case '\x7f':
    if (cons.e != cons.w) {
      cons.e--;
      consputc(BACKSPACE);
    }
    break;
  default:
    if (c != 0 && cons.e - cons.r < INPUT_BUF) {
      c = (c == '\r') ? '\n' : c;

      // echo back to the user.
      consputc(c);

      // store for consumption by consoleread().
      cons.buf[cons.e++ % INPUT_BUF] = c;

      if (c == '\n' || c == C('D') || cons.e == cons.r + INPUT_BUF) {
        // wake up consoleread() if a whole line (or end-of-file)
        // has arrived.

        save_to_cons_history(cons.e - cons.w);

        cons.w = cons.e;
        wakeup(&cons.r);
      }
    }
    break;
  }

  release(&cons.lock);
}

void consoleinit(void) {
  initlock(&cons.lock, "cons");

  uartinit();

  // connect read and write system calls
  // to consoleread and consolewrite.
  devsw[CONSOLE].read = consoleread;
  devsw[CONSOLE].write = consolewrite;
}
